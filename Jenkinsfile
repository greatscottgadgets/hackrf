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
                retry(3) {
                    sh './ci-scripts/test-firmware-program.sh'
                }
                sh './ci-scripts/test-firmware-flash.sh'
                sh 'python3 ci-scripts/test-debug.py'
                retry(3) {
                    sh 'python3 ci-scripts/test-transfer.py tx'
                }
                retry(3) {
                    sh 'python3 ci-scripts/test-transfer.py rx'
                }
                sh './ci-scripts/configure-hubs.sh --off'
                sh 'python3 ci-scripts/test-sgpio-debug.py'
                sh './ci-scripts/configure-hubs.sh --reset'
            }
        }
    }
    post {
        always {
            cleanWs(cleanWhenNotBuilt: false,
                    deleteDirs: true,
                    disableDeferredWipeout: true,
                    notFailBuild: true)
        }
    }
}
