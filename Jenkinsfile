import org.jenkinsci.plugins.workflow.steps.FlowInterruptedException

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
                            -v /tmp/req_pipe:/tmp/req_pipe -v /tmp/res_pipe:/tmp/res_pipe'''
                }
            }
            steps {
                sh './ci-scripts/install_host.sh'
                sh './ci-scripts/build_h1_firmware.sh'
                script {
                    allOff();
                }
                retry(3) {
                    script {
                        try {
                            // Allow 20 seconds for the USB hub port power server to respond
                            timeout(time: 20, unit: 'SECONDS') {
                                sh 'hubs h1_eut reset'
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "USB hub port power server command timeout reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    sh './ci-scripts/test_host.sh'
                }
                retry(3) {
                    script {
                        try {
                            // Allow 20 seconds for the USB hub port power server to respond
                            timeout(time: 20, unit: 'SECONDS') {
                                sh 'hubs h1_tester h1_eut reset'
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "USB hub port power server command timeout reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    script {
                        try {
                            // Allow 5 minutes for the test to run
                            timeout(time: 5, unit: 'MINUTES') {
                                sh '''python3 ci-scripts/hackrf_test.py --ci --log log \
                                    --hostdir host/build/hackrf-tools/src/ \
                                    --fwupdate firmware/hackrf_usb/build/ \
                                    --tester 0000000000000000325866e629a25623 \
                                    --eut RunningFromRAM --unattended --rev r4'''
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "HackRF One Test timeout limit reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
                            throw err
                        }
                    }
                    script {
                        try {
                            // Allow 20 seconds for the USB hub port power server to respond
                            timeout(time: 20, unit: 'SECONDS') {
                                sh 'hubs all off'
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "USB hub port power server command timeout reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
                            throw err
                        }
                    }
                }
                retry(3) {
                    script {
                        try {
                            // Allow 20 seconds for the USB hub port power server to respond
                            timeout(time: 20, unit: 'SECONDS') {
                                sh 'hubs h1_eut reset'
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "USB hub port power server command timeout reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
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
                    -v /tmp/req_pipe:/tmp/req_pipe -v /tmp/res_pipe:/tmp/res_pipe'''
                }
            }
            steps {
                sh './ci-scripts/install_host.sh'
                sh './ci-scripts/build_hpro_firmware.sh'
                script {
                    allOff();
                }
                retry(3) {
                    script {
                        try {
                            // Allow 20 seconds for the USB hub port power server to respond
                            timeout(time: 20, unit: 'SECONDS') {
                                sh 'hubs hpro_eut reset'
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "USB hub port power server command timeout reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
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
                            timeout(time: 20, unit: 'SECONDS') {
                                sh 'hubs hpro_tester hpro_eut reset'
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "USB hub port power server command timeout reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
                            throw err
                        }
                    }
                    sh 'sleep 1s'
                    script {
                        try {
                            // Allow 5 minutes for the test to run
                            timeout(time: 5, unit: 'MINUTES') {
                                sh '''python3 ci-scripts/hackrf_pro_test.py --ci --log log \
                                    --hostdir host/build/hackrf-tools/src \
                                    --fwupdate firmware/hackrf_usb/build \
                                    --tester 0000000000000000a06063c82338145f \
                                    --eut RunningFromRAM -p --rev r1.2'''
                            }
                        } catch (FlowInterruptedException err) {
                            // Check if the cause was specifically an exceeded timeout
                            def cause = err.getCauses().get(0)
                            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                                echo "HackRF Pro Test timeout limit reached."
                                throw err // Re-throw the exception to fail the build
                            } else {
                                echo "Build interrupted for another reason."
                                throw err // Re-throw the exception to fail the build
                            }
                        } catch (Exception err) {
                            echo "An unrelated error occurred: ${err.getMessage()}"
                            throw err
                        }
                    }
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

def allOff() {
    try {
        // Allow 20 seconds for the USB hub port power server to respond
        timeout(time: 20, unit: 'SECONDS') {
            sh 'hubs all off'
        }
    } catch (FlowInterruptedException err) {
        // Check if the cause was specifically an exceeded timeout
        def cause = err.getCauses().get(0)
        if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
            echo "USB hub port power server command timeout reached."
            throw err // Re-throw the exception to fail the build
        } else {
            echo "Build interrupted for another reason."
            throw err // Re-throw the exception to fail the build
        }
    } catch (Exception err) {
        echo "An unrelated error occurred: ${err.getMessage()}"
        throw err
    }
}
