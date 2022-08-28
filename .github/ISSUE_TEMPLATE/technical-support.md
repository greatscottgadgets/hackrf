name: Technical Support Request
description: File a technical support request
title: "[Tech Support]: "
labels: ["technical support"]
assignees:
  - straithe
body:
  - type: markdown
    attributes:
      value: |
        Thank you for taking the time to fill out this technical support request form! Please note that technical support requests can expect a response time of two weeks.  
  - type: dropdown
    id: troubleshooting documentation
    attributes: 
      label: Have you read the HackRF [troubleshooting documentation](https://hackrf.readthedocs.io/en/latest/troubleshooting.html)?
      options:
        - yes
        - no
    validations:
      required: true
  - type: textarea
    id: expected outcome
    attributes:
      label: What outcome were you hoping for?
      placeholder: Please be detailed in what you expected to happen
    validations:
      required: true
  - type: textarea
    id: actual outcome
    attributes:
      label: What outcome actually happened? 
      placeholder: Please be detailed in what did happen
    validations:
      required: true
  - type: checkboxes
    id: operating systems
    attributes:
      label: What operating systems are you seeing the problem on?
      multiple: true
      options: 
        - Linux
        - Windows
        - MacOS
        - other
    validations:
      required: true
  - type: textarea
    id: hackrf_info output
    attributes:
      label: What is the output of ```hackrf_info```?
      description: Please put the output of the command in the box below or indicate N/A. 
      placeholder: ```hackrf_info version: unknown
        libhackrf version: unknown (0.5)
        Found HackRF
        Index: 0
        Serial number: 0000000000000000325866e6211d5123
        Board ID Number: 2 (HackRF One)
        Firmware Version: git-9a1c0e4e (API:1.06)
        Part ID Number: 0xa000cb3c 0x004c4752```
    validations:
      required: true
  - type: textarea
    id: third-party software
    attributes:
      label: Are you using any third-party software?
      description: Please list the software you are using with your HackRF or indicate N/A. Please list version numbers where possible. 
      placeholder: GNU Radio, GQRX, etc. 
    validations:
      required: true
  - type: textarea
    id: third-party software
    attributes:
      label: Are you using any third-party hardware? 
      description: Please list any hardware you are using with your HackRF or indicate N/A. 
      placeholder: portapack, specialized antenna, etc. 
    validations:
      required: true
