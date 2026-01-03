# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'nRF5340 DK Matter Smart Light'
copyright = '2024-2026, Smart Home Project'
author = 'Smart Home Development Team'
release = '1.0.0'
version = '1.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.intersphinx',
    'sphinx.ext.autodoc',
    'sphinx.ext.todo',
    'sphinx.ext.viewcode',
]

# Try to add PlantUML extension if available
plantuml_available = False
try:
    import sphinxcontrib.plantuml
    extensions.append('sphinxcontrib.plantuml')
    plantuml_available = True
    
    # PlantUML configuration - fallback to plantuml command if jar not found
    import shutil
    if shutil.which('plantuml'):
        plantuml = 'plantuml'
    else:
        plantuml = 'java -jar /usr/share/plantuml/plantuml.jar'
    plantuml_output_format = 'svg'
except ImportError:
    # PlantUML not available - will use ASCII fallback diagrams
    pass

# Make PlantUML availability known to templates
html_context = {
    'plantuml_available': plantuml_available
}

templates_path = ['_templates']

# Exclude patterns - do not include README, CI/CD, or build artifacts
exclude_patterns = [
    '_build', 
    '_build_sphinx', 
    'Thumbs.db', 
    '.DS_Store',
    'README.md',
    'README.rst',
    '../.github/**',
    '../.gitlab-ci.yml',
    '**/.git/**',
    '**/node_modules/**',
    '**/build/**',
    '**/build_app/**',
    '**/build_net/**',
]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

html_theme_options = {
    'navigation_depth': 4,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'titles_only': False,
}

# Custom CSS for diagram scaling
html_css_files = [
    'custom.css',
]

# -- Options for Intersphinx -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html

intersphinx_mapping = {
    'zephyr': ('https://docs.zephyrproject.org/latest/', None),
    'matter': ('https://project-chip.github.io/connectedhomeip-doc/', None),
}
