pipeline {
    agent any
    stages {
        stage('Build Docker Image') {
            steps {
                sh 'docker build -t hackrf https://github.com/greatscottgadgets/hackrf.git'
            }
        }
        stage('Test Suite') {
            agent {
                docker {
                    image 'hackrf'
                    reuseNode true
                    args '--group-add=20 --group-add=46 --device-cgroup-rule="c 189:* rmw" --device-cgroup-rule="c 166:* rmw" -v /dev/bus/usb:/dev/bus/usb -e TESTER=0000000000000000325866e629a25623 -e EUT=RunningFromRAM'
                }
            }
            steps {
                sh './ci-scripts/install-host.sh'
                sh './ci-scripts/install-firmware.sh'
                sh 'hubs all off'
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
                sh 'hubs all off'
                sh 'python3 ci-scripts/test-sgpio-debug.py'
                sh 'hubs all reset'
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
