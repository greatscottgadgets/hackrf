import sphinx_rtd_theme

extensions = [
    'sphinx_rtd_theme'
]

rst_prolog = """
.. .include:: htime.txt

"""

# -- Project information -----------------------------------------------------

project = 'HTime'
copyright = '2025, Fabrizio Pollastri'
author = 'Fabrizio Pollastri <mxgbot@gmail.com>'

version = '0.1.0'
release = '0.1.0'


# -- General configuration ---------------------------------------------------

extensions = [
  'sphinx.ext.autodoc'
]

templates_path = ['_templates']
exclude_patterns = ['_build']
source_suffix = '.rst'
master_doc = 'index'
language = 'en'
exclude_patterns = []
pygments_style = None


# -- Options for HTML output -------------------------------------------------
# run pip install sphinx_rtd_theme if you get sphinx_rtd_theme errors
html_theme = "sphinx_rtd_theme"
html_static_path = ['./']
html_css_files = ['status.css','htime.css']
