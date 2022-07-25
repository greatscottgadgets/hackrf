name: Feature Request 
description: File a feature request
title: "[Feature Request]: "
labels: ["enhancement"]
body:
  - type: markdown
    attributes:
      value: |
        Thank you for taking the time to fill out this feature request form! 
  - type: textarea
    id: feature request
    attributes:
      label: What feature are you requesting?
      description: Please be as detailed as possible with your feature request.
    validations:
      required: true
  - type: textarea
    id: problem solved
    attributes:
      label: What problem are you hoping this feature will solve? 
      description: Please be as detailed as possible with what issure you are hoping to have solved.
    validations:
      required: true
  - type: textarea
    id: benefit
    attributes:
      label: What benefit does this feature provide?
      description: Specify the impact that can be expected from this feature. (Saved time, new functionality, etc.) 
    validations:
      required: true
  - type: textarea
    id: reach
    attributes:
      label: Who will benefit from the new feature?
      description: Describe how many users, and which users, you think will be impacted by this feature. 
    validations:
      required: true
  - type: textarea
    id: uniqueness
    attributes:
      label: How do current features fail to solve the problem you are experiencing?
      description: Specify the uniqueness of the feature being requested. 
    validations:
      required: true
  - type: textarea
    id: extra detail
    attributes:
      label: Are there any other details you would like to share? 
