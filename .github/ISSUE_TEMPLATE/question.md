name: Question  
description: Ask a question not covered by current documentation
title: "[Question]: "
labels: ["question"]
body:
  - type: textarea
    id: question
    attributes:
      label: What would you like to know?
    validations:
      required: true