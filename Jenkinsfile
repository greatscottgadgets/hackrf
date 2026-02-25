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
                    args '--group-add=20 --group-add=46 --device-cgroup-rule="c 189:* rmw" --device-cgroup-rule="c 166:* rmw" -v /dev/bus/usb:/dev/bus/usb -e TESTER=0000000000000000325866e629a25623 -e EUT=RunningFromRAM -v /tmp/req_pipe:/tmp/req_pipe -v /tmp/res_pipe:/tmp/res_pipe'
                }
            }
            steps {
                sh './ci-scripts/install-host.sh'
                sh './ci-scripts/install-firmware.sh'
                sh 'hubs all off'
                retry(3) {
                    sh 'hubs h1_eut reset'
                    sh 'sleep 1s'
                    sh './ci-scripts/test-host.sh'
                }
                sh 'hubs h1_tester h1_eut on'
                sh 'sleep 1s'
                sh 'python3 ci-scripts/hackrf_test.py --unattended --ci --log log --rev r4 --manufacturer --hostdir host/build/hackrf-tools/src/ --fwupdate firmware/hackrf_usb/build/ --tester 0000000000000000325866e629a25623 --eut RunningFromRAM'
                sh 'hubs all off'
                retry(3) {
                    sh 'hubs h1_eut reset'
                    sh 'sleep 1s'
                    sh 'python3 ci-scripts/test-sgpio-debug.py'
                }
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
