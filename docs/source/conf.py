# -- Project information -----------------------------------------------------

project = 'HackRF'
copyright = '2021, Great Scott Gadgets'
author = 'Great Scott Gadgets'

# The short X.Y version
version = ''
# The full version, including alpha/beta/rc tags
release = ''


# -- General configuration ---------------------------------------------------

extensions = [
  'sphinx.ext.autodoc'
]

templates_path = ['_templates']
exclude_patterns = ['_build']
source_suffix = '.rst'
master_doc = 'index'
language = None
exclude_patterns = []
pygments_style = None


# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_css_files = ['status.css']

