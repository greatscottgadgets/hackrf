pipeline {
    agent {
        dockerfile {
            args '--group-add=46 --device-cgroup-rule="c 189:* rmw" -v /dev/bus/usb:/dev/bus/usb'
        }
    }
    stages {
        stage('Build (Host)') {
            steps {
                sh './ci-scripts/install-host.sh'
            }
        }
        stage('Build (Firmware)') {
            steps {
                sh './ci-scripts/install-firmware.sh'
            }
        }
        stage('Test') {
            steps {
                sh './ci-scripts/configure-hubs.sh --off'
                retry(3) {
                    sh './ci-scripts/test-host.sh'
                }
                sh 'usbhub --disable-i2c --hub 624C power state --port 2 --reset'
                sh 'python3 ci-scripts/hackrf_test.py --ci --rev r4 --manufacturer --hostdir host/build/hackrf-tools/src/ --fwupdate firmware/hackrf_usb/build/ --tester 0000000000000000325866e629a25623 --eut RunningFromRAM'
            }
        }
    }
    post {
        always {
            sh './ci-scripts/configure-hubs.sh --reset'
            sh 'rm -rf testing-venv/'
            cleanWs(cleanWhenNotBuilt: false,
                    deleteDirs: true,
                    disableDeferredWipeout: true,
                    notFailBuild: true)
        }
    }
}
