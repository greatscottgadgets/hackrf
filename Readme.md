# HackRF

This repository contains hardware designs and software for HackRF,
a low cost, open source Software Defined Radio platform.

![HackRF One](https://raw.github.com/mossmann/hackrf/master/docs/images/HackRF-One-fd0-0009.jpeg)

(photo by fd0 from https://github.com/fd0/hackrf-one-pictures)

principal author: Michael Ossmann <mike@ossmann.com>

Information on HackRF and purchasing HackRF: https://greatscottgadgets.com/hackrf/

--------------------

# Documentation

Documentation for HackRF can be viewed on [Read the Docs](https://hackrf.readthedocs.io/en/latest/). The raw documentation files for HackRF are in the [docs folder](https://github.com/mossmann/hackrf/tree/master/docs) in this repository and can be built locally by installing [Sphinx Docs](https://www.sphinx-doc.org/en/master/usage/installation.html) and running `make html`. Documentation changes can be submitted through pull request and suggestions can be made as GitHub issues. 

To create a PDF of the HackRF documentation from the HackRF repository while on Ubuntu:
* run `sudo apt install latexmk texlive-latex-extra`
* navigate to hackrf/docs on command line
* run the command `make latex`
* run the command `make latexpdf`

--------------------

# Getting Help

Before asking for help with HackRF, check to see if your question is listed in the [FAQ](https://hackrf.readthedocs.io/en/latest/faq.html).

For assistance with HackRF general use or development, please look at the [issues on the GitHub project](https://github.com/greatscottgadgets/hackrf/issues). This is the preferred place to ask questions so that others may locate the answer to your question in the future.

We invite you to join our community discussions on [Discord](https://discord.gg/rsfMw3rsU8). Note that while technical support requests are welcome here, we do not have support staff on duty at all times. Be sure to also submit an issue on GitHub if you've found a bug or if you want to ensure that your request will be tracked and not overlooked.

If you wish to see past discussions and questions about HackRF, you may also view the [mailing list archives](https://pairlist9.pair.net/pipermail/hackrf-dev/).

GitHub issues on this repository that are labelled "technical support" by Great Scott Gadgets employees can expect a response time of two weeks. We currently do not have expected response times for other GitHub issues or pull requests for this repository. 
