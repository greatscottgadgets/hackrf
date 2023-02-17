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
                sh 'python3 ci-scripts/hackrf_test.py --ci --manufacturer --fwupdate firmware/hackrf_usb/build/ --tester 0000000000000000325866e629822923 --eut RunningFromRAM'
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
