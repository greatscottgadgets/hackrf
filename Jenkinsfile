pipeline {
    agent any
    stages {
        stage('Build Docker Image') {
            steps {
                sh 'docker build -t hackrf https://github.com/greatscottgadgets/hackrf.git'
            }
        }
        stage('Test HackRF One') {
            agent {
                docker {
                    image 'hackrf'
                    reuseNode true
                    args '''--group-add=20 --group-add=46 --device-cgroup-rule="c 189:* rmw" \
                            --device-cgroup-rule="c 166:* rmw" -v /dev/bus/usb:/dev/bus/usb \
                            -e TESTER=0000000000000000325866e629a25623 -e EUT=RunningFromRAM \
                            -v /tmp/req_pipe:/tmp/req_pipe -v /tmp/res_pipe:/tmp/res_pipe'''
                }
            }
            steps {
                sh './ci-scripts/install_host.sh'
                sh './ci-scripts/build_h1_firmware.sh'
                script {
                    try {
                        // Allow 10 seconds for the USB hub port power server to respond
                        timeout(time: 10, unit: 'SECONDS') {
                            sh 'hubs all off'
                        }
                    } catch (err) {
                        echo "USB hub port power server command timeout reached."
                        throw err
                    }
                }
                retry(3) {
                    script {
                        try {
                            // Allow 10 seconds for the USB hub port power server to respond
                            timeout(time: 10, unit: 'SECONDS') {
                                sh 'hubs h1_eut reset'
                            }
                        } catch (err) {
                            echo "USB hub port power server command timeout reached."
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    sh './ci-scripts/test_host.sh'
                }
                retry(3) {
                    script {
                        try {
                            // Allow 10 seconds for the USB hub port power server to respond
                            timeout(time: 10, unit: 'SECONDS') {
                                sh 'hubs h1_tester h1_eut reset'
                            }
                        } catch (err) {
                            echo "USB hub port power server command timeout reached."
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    sh '''python3 ci-scripts/hackrf_test.py --ci --log log \
                        --hostdir host/build/hackrf-tools/src/ \
                        --fwupdate firmware/hackrf_usb/build/ \
                        --tester 0000000000000000325866e629a25623 \
                        --eut RunningFromRAM --unattended --rev r4'''
                    sh 'hubs all off'
                }
                retry(3) {
                    script {
                        try {
                            // Allow 10 seconds for the USB hub port power server to respond
                            timeout(time: 10, unit: 'SECONDS') {
                                sh 'hubs h1_eut reset'
                            }
                        } catch (err) {
                            echo "USB hub port power server command timeout reached."
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    sh 'python3 ci-scripts/test_sgpio_debug.py'
                }
            }
        }
        stage('Test HackRF Pro') {
            agent {
                docker {
                    image 'hackrf'
                    reuseNode true
                    args '''--group-add=20 --group-add=46 --device-cgroup-rule="c 189:* rmw" \
                    --device-cgroup-rule="c 166:* rmw" -v /dev/bus/usb:/dev/bus/usb \
                    -e TESTER=0000000000000000325866e629a25623 -e EUT=RunningFromRAM \
                    -v /tmp/req_pipe:/tmp/req_pipe -v /tmp/res_pipe:/tmp/res_pipe'''
                }
            }
            steps {
                sh './ci-scripts/install_host.sh'
                sh './ci-scripts/build_hpro_firmware.sh'
                script {
                    try {
                        // Allow 10 seconds for the USB hub port power server to respond
                        timeout(time: 10, unit: 'SECONDS') {
                            sh 'hubs all off'
                        }
                    } catch (err) {
                        echo "USB hub port power server command timeout reached."
                        throw err
                    }
                }
                retry(3) {
                    script {
                        try {
                            // Allow 10 seconds for the USB hub port power server to respond
                            timeout(time: 10, unit: 'SECONDS') {
                                sh 'hubs hpro_eut reset'
                            }
                        } catch (err) {
                            echo "USB hub port power server command timeout reached."
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    sh './ci-scripts/test_host.sh'
                }
                retry(3) {
                    script {
                        try {
                            // Allow 10 seconds for the USB hub port power server to respond
                            timeout(time: 10, unit: 'SECONDS') {
                                sh 'hubs hpro_tester hpro_eut reset'
                            }
                        } catch (err) {
                            echo "USB hub port power server command timeout reached."
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    sh '''python3 ci-scripts/hackrf_pro_test.py --ci --log log \
                        --hostdir host/build/hackrf-tools/src \
                        --fwupdate firmware/hackrf_usb/build \
                        --tester 0000000000000000a06063c82338145f \
                        --eut RunningFromRAM -p --rev r1.2'''
                    script {
                        try {
                            // Allow 10 seconds for the USB hub port power server to respond
                            timeout(time: 10, unit: 'SECONDS') {
                                sh 'hubs all off'
                            }
                        } catch (err) {
                            echo "USB hub port power server command timeout reached."
                            throw err
                        }
                    }
                }
                retry(3) {
                    script {
                        try {
                            // Allow 10 seconds for the USB hub port power server to respond
                            timeout(time: 10, unit: 'SECONDS') {
                                sh 'hubs hpro_eut reset'
                            }
                        } catch (err) {
                            echo "USB hub port power server command timeout reached."
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    sh 'python3 ci-scripts/test_sgpio_debug.py'
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
