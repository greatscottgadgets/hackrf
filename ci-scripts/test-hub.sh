#!/bin/bash
source testing-venv/bin/activate
echo "usbhub id:"
usbhub id
echo "usbhub power state:"
usbhub power state
deactivate