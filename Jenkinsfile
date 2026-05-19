import org.jenkinsci.plugins.workflow.steps.FlowInterruptedException

def docker_args = '''--group-add=20 --group-add=46 --device-cgroup-rule="c 189:* rmw" \
                    --device-cgroup-rule="c 166:* rmw" -v /dev/bus/usb:/dev/bus/usb \
                    -v /tmp/req_pipe:/tmp/req_pipe -v /tmp/res_pipe:/tmp/res_pipe'''

def h1_test = '''python3 ci-scripts/hackrf_test.py --ci --log log \
                    --hostdir host/build/hackrf-tools/src/ \
                    --fwupdate firmware/hackrf_usb/build/ \
                    --tester 0000000000000000325866e629a25623 \
                    --eut RunningFromRAM --unattended --rev r4'''

def hpro_test = '''python3 ci-scripts/hackrf_pro_test.py --ci --log log \
                    --hostdir host/build/hackrf-tools/src \
                    --fwupdate firmware/hackrf_usb/build \
                    --tester 0000000000000000a06063c82338145f \
                    --eut RunningFromRAM -p --rev r1.2'''

pipeline {
    agent any
    stages {
        stage('Build Docker Image') {
            options {
                timeout(time: 20, unit: 'MINUTES')
            }
            steps {
                sh 'docker build -t hackrf https://github.com/greatscottgadgets/hackrf.git'
            }
        }
        stage('Test HackRF One with BOARD=HACKRF_ONE') {
            agent {
                docker {
                    image 'hackrf'
                    reuseNode true
                    args docker_args
                }
            }
            options {
                timeout(time: 20, unit: 'MINUTES')
            }
            steps {
                runCommand("Install Host Tools", './ci-scripts/install_host.sh', 3, 1, 'MINUTES')
                runCommand("Build HackRF One Firmware", './ci-scripts/build_firmware.sh HACKRF_ONE', 3, 1, 'MINUTES')
                lock('HIL_hubs') {
                    script {
                        allOff()
                        runTest("Check Host", 'h1_eut', './ci-scripts/test_host.sh')
                        runTest("HackRF One HIL Test", 'h1_tester h1_eut', h1_test)
                        runTest("SGPIO Debug Test", 'h1_eut', 'python3 ci-scripts/test_sgpio_debug.py')
                    }
                }
            }
        }
        stage('Test HackRF One with BOARD=UNIVERSAL') {
            agent {
                docker {
                    image 'hackrf'
                    reuseNode true
                    args docker_args
                }
            }
            options {
                timeout(time: 20, unit: 'MINUTES')
            }
            steps {
                runCommand("Install Host Tools", './ci-scripts/install_host.sh', 3, 1, 'MINUTES')
                runCommand("Build Universal Firmware", './ci-scripts/build_firmware.sh UNIVERSAL', 3, 1, 'MINUTES')
                lock('HIL_hubs') {
                    script {
                        allOff()
                        runTest("Check Host", 'h1_eut', './ci-scripts/test_host.sh')
                        runTest("HackRF One HIL Test", 'h1_tester h1_eut', h1_test)
                        runTest("SGPIO Debug Test", 'h1_eut', 'python3 ci-scripts/test_sgpio_debug.py')
                    }
                }

            }
        }
        stage('Test HackRF Pro with BOARD=PRALINE') {
            agent {
                docker {
                    image 'hackrf'
                    reuseNode true
                    args "$docker_args"
                }
            }
            options {
                timeout(time: 20, unit: 'MINUTES')
            }
            steps {
                runCommand("Install Host Tools", './ci-scripts/install_host.sh', 3, 1, 'MINUTES')
                runCommand("Build Praline Firmware", './ci-scripts/build_firmware.sh PRALINE', 3, 1, 'MINUTES')
                lock('HIL_hubs') {
                    script {
                        allOff()
                        runTest("Check Host", 'hpro_eut', './ci-scripts/test_host.sh')
                        runTest("HackRF Pro HIL Test", 'hpro_tester hpro_eut', hpro_test)
                    }
                }
            }
        }
        stage('Test HackRF Pro with BOARD=UNIVERSAL') {
            agent {
                docker {
                    image 'hackrf'
                    reuseNode true
                    args "$docker_args"
                }
            }
            options {
                timeout(time: 20, unit: 'MINUTES')
            }
            steps {
                runCommand("Install Host Tools", './ci-scripts/install_host.sh', 3, 1, 'MINUTES')
                runCommand("Build Universal Firmware", './ci-scripts/build_firmware.sh UNIVERSAL', 3, 1, 'MINUTES')
                lock('HIL_hubs') {
                    script {
                        allOff()
                        runTest("Check Host", 'hpro_eut', './ci-scripts/test_host.sh')
                        runTest("HackRF Pro HIL Test", 'hpro_tester hpro_eut', hpro_test)
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
    // Allow up to 3 retries, 20 seconds each, for the USB hub port power server to respond appropriately
    runCommand('USB hub port power server command', "hubs all off", 3, 20, 'SECONDS')
}

def reset(devices) {
    // Allow up to 3 retries, 20 seconds each, for the USB hub port power server to respond appropriately
    runCommand('USB hub port power server command', "hubs ${devices} reset", 3, 20, 'SECONDS')
}

def runCommand(title, cmd, retries, time, unit) {
    retry(retries) {
        try {
            timeout(time: time, unit: unit) {
                sh "${cmd}"
            }
        } catch (FlowInterruptedException err) {
            // Check if the cause was specifically an exceeded timeout
            def cause = err.getCauses().get(0)
            if (cause instanceof org.jenkinsci.plugins.workflow.steps.TimeoutStepExecution.ExceededTimeout) {
                echo "${title} timeout reached."
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

def runTest(title, devices, cmd) {
    retry(3) {
        // reset() retains it's own internal retries
        reset(devices)
        sh 'sleep 1s'
        // run the test with 0 internal retries and 3 external retries to ensure resets between runs
        runCommand(title, cmd, 0, 5, 'MINUTES')
    }
}
