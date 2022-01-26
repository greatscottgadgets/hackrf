pipeline {
    agent { 
        dockerfile {
            args '--group-add=46 --privileged -v /dev/bus/usb:/dev/bus/usb'
        }
    }
    options {
        throttleJobProperty(
            categories: ['single-build-throttle'],
            throttleEnabled: true,
            throttleOption: 'category'
        )
    }
    environment {
        GIT_COMMITER_NAME = 'CI Person'
        GIT_COMMITER_EMAIL = 'ci@greatscottgadgets.com'
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
                sh './ci-scripts/test-hub.sh'
                retry(3) {
                    sh './ci-scripts/test-host.sh'
                }
                retry(3) {
                    sh './ci-scripts/test-firmware-program.sh'
                }
                sh './ci-scripts/test-firmware-flash.sh'
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
