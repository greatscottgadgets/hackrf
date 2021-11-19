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
            echo 'One way or another, I have finished'
            sh 'rm -rf testing-venv/'
            // Clean after build
            cleanWs(cleanWhenNotBuilt: false,
                    deleteDirs: true,
                    disableDeferredWipeout: true,
                    notFailBuild: true)
        }
    }
}
