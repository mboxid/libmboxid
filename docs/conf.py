# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'libmboxid'
copyright = '2024, Franz Hollerer'
author = 'Franz Hollerer'
release = '0.1.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['breathe', 'exhale']

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Set up the breathe extension --------------------------------------------

breathe_projects = {"libmboxid": "./_build/_doxygen/xml"}
breathe_default_project ="libmboxid"

# -- Set up the exhale extension ---------------------------------------------

exhale_args = {
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "doxygenStripFromPath":  "../include/mboxid",
    "rootFileTitle":         "Library API",
    "createTreeView":        True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin":    "INPUT = ../include/mboxid"
}


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
html_theme_options = {
    'body_min_width': '50em',
    'body_max_width': 'none',
}
html_static_path = ['_static']
html_logo = './mboxid_logo.svg'
primary_domain = 'cpp'
highlight_language = 'cpp'

# -- Options for LaTeX output ------------------------------------------------

latex_logo = './mboxid_logo.png'
