# Tool to extract font data, save to bitmap file, and apply modifications
# to source code after editing the bitmap.

from __future__ import print_function
from matplotlib.pyplot import imread, imsave, imshow, show
import numpy as np
import sys
import re

if len(sys.argv) != 3:
    print("Usage: %s <font name> { show | extract | insert}" % sys.argv[0])
    sys.exit(1)

font_name = sys.argv[1]
mode = sys.argv[2]

# Where to find the font data - may need updated if code has changed.
source_file = 'firmware/common/ui_portapack.c'
data_marker = f'static const uint8_t {font_name}_glyph_data[] = {{'
data_offset = 1  # Data starts this number of lines after start marker.
end_pattern = '(.*)};$' # Indicates end of font data.
def_marker = f'static const ui_font_t {font_name} ='
def_offset = 1
def_pattern = '^\t{{(?P<width>\\d+), (?P<height>\\d+)}'

# Image filename used in extract and insert modes.
image_file = f'{font_name}.png'

# Get the literals used to populate the font array in the source file
lines = [line.rstrip() for line in open(source_file).readlines()]
try:
    start = lines.index(data_marker) + data_offset
except ValueError:
    print(f"Could not find glyph data for font '{font_name}'")
    sys.exit(1)

data = str()
for line in lines[start:]:
    if match := re.match(end_pattern, line):
        data += match.group(1)
        break
    else:
        data += line

# Get the width and height from the font definition.
try:
    def_start = lines.index(def_marker)
except ValueError:
    print(f"Could not find definition for font '{font_name}'")
    sys.exit(1)

def_match = re.match(def_pattern, lines[def_start + def_offset])

if def_match is None:
    print(f"Failed to parse definition of '{font_name}'")

width = int(def_match.group('width'))
height = int(def_match.group('height'))

# Eval the literals to get the values into a numpy array
packed = eval("np.array([%s], np.uint8)" % data)

# Reorganise into a monochrome image.
chars_per_row = 5
unpacked = np.unpackbits(packed)
bitmaps = (unpacked
    .reshape(-1, height, width // 8, 8)[:,:,::-1,:]
    .reshape(-1, height, width)[:,:,::-1]
)
indices = np.arange(len(bitmaps)).reshape(-1, chars_per_row)
image = np.block([[bitmaps[idx] for idx in row] for row in indices])

if mode == 'show':
    # Display font image
    imshow(image, cmap='binary')
    show()

elif mode == 'extract':
    # Save font image
    imsave(image_file, image, format='png', cmap='binary')

elif mode == 'insert':
    # Read in modified font image
    image = imread(image_file)[:,:,0].astype(bool)

    # Reorganise back to original order
    rows = image.reshape(-1, height, image.shape[1])
    single_row_image = np.hstack(rows)
    num_chars = single_row_image.shape[1] // width
    bitmaps = np.array(np.split(single_row_image, num_chars, 1))
    unpacked = (bitmaps
        [:,:,::-1].reshape(-1, height, width // 8, 8)
        [:,:,::-1,:].reshape(-1)
    )
    packed = ~np.packbits(unpacked)

    # Replace lines of file in same format as used before
    bytes_per_row = 13
    split_indices = np.arange(bytes_per_row, len(packed), bytes_per_row)
    for i, group in enumerate(np.split(packed, split_indices)):
        line = ("\t0x%02x," + " 0x%02x," * (len(group) - 1)) % tuple(group)
        if len(group) < bytes_per_row:
            line = line[:-1] + "};"
        lines[start + i] = line

    # Write out modified source file
    open(source_file, 'w').writelines([line + "\n" for line in lines])
