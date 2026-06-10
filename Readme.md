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

Before asking for help with HackRF, check to see if your question is listed on the [troubleshooting page](https://hackrf.readthedocs.io/en/latest/troubleshooting.html).

For assistance with HackRF general use or development, please look at the [issues on the GitHub project](https://github.com/greatscottgadgets/hackrf/issues). This is the preferred place to ask questions so that others may locate the answer to your question in the future.

We invite you to join our community discussions on [Discord](https://discord.gg/rsfMw3rsU8). Note that while technical support requests are welcome here, we do not have support staff on duty at all times. Be sure to also submit an issue on GitHub if you've found a bug or if you want to ensure that your request will be tracked and not overlooked.

If you wish to see past discussions and questions about HackRF, you may also view the [mailing list archives](https://pairlist9.pair.net/pipermail/hackrf-dev/).

GitHub issues on this repository that are labelled "technical support" by Great Scott Gadgets employees can expect a response time of two weeks. We currently do not have expected response times for other GitHub issues or pull requests for this repository. 

Update README.md with minimal ethics#1

What about the application of the Solomonoff induction
to the RF sensing of human brain?
https://en.wikipedia.org/wiki/Talk:Solomonoff%27s_theory_of_inductive_inference

1. If we assume that the algorithmic complexity
of neural processes is relatively small
(you could take a look at the Potapov monography
for some arguments)

2. As far as I know a lot of neural processes in human brain are electric (or electro-chemistry)
in nature. They have a little power and a small
frequency (~ 1-1000 Hz).
So they emit very low frequency radio waves.

3. You can detect those radio waves on
small antennas if the impedance of such antennas
is matched. This is basically the citation
from the book on electrodynamics.
I uploaded one such book on Twirpix site.

4. So you can create a lot of such receivers
- microstrip filter to filter very high frequencies
- impedance matched microstrip antenna
- resistor for noise for oversampling
- very fast comparator to sample signal
in a very large array on a chip using
standard CMOS or some sort of full-custom process
(maybe even with some new materials)

5. BreamForming and large arrays of digital correlators with sub-mm positioning accuracy
could be achived.

so it seems there is no theoretical
obstacles to implement some sort of
RF human brain sensing or even control
if you can implement reverse structure
with array of a large amount of RF amplifiers
with sub-mm beam forming accuracy

BTW I wrote that remark:
https://en.wikipedia.org/wiki/Talk:Solomonoff%27s_theory_of_inductive_inference

What about the application of the Solomonoff induction to the person to person information exchange?

It seems the minimum description length principle could be used to deduce minimum ethics. At least you can compare ethics by the length. What about the following question: how to shoot met-art girl in the dark underground? ~2026-24709-41 (talk) 16:43, 8 June 2026 (UTC)

at least we can ask a question: does russian met-art model from a pic want some money?
and what obstacles do that guess so hard?
why it is so hard to contact her directly?
she lose money, interesting person lose opportunity to make some photos.

also, it seems the economy without coordination primitives based on physics or mathematics
like gold or maybe Bitcoin or maybe some sort of other physical objects or processes
is just a chaos.
Economic calculation based on fiat money is impossible just from scientific methodology
https://en.wikipedia.org/wiki/Talk:Solomonoff%27s_theory_of_inductive_inference

https://transitional-writes.dreamwidth.org/44972.html
Kirill Abramovich aka wieker @ linux.org.ru
Samara, Russia

and the most important question:
what about the minimal ethics deduced from the MDL principle?
is it exist?

what was the obstacle between russian met-art girls and engineers?
