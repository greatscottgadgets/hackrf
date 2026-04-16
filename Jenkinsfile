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
                sh './ci-scripts/install_host.sh'
                sh './ci-scripts/build_firmware.sh HACKRF_ONE'
                script {
                    allOff()
                    reset('h1_eut')
                }
                sh 'sleep 1s'
                retry(3) {
                    sh './ci-scripts/test_host.sh'
                }
                script {
                    reset('h1_tester h1_eut')
                }
                sh 'sleep 1s'
                script {
                    // Allow up to 3 retries 5 minutes each for the HIL test
                    runCommand(3, 5, 'MINUTES', "HackRF One Test", h1_test)
                }
                script {
                    allOff()
                    reset('h1_eut')
                }
                sh 'sleep 1s'
                retry(3) {
                    sh 'python3 ci-scripts/test_sgpio_debug.py'
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
                sh './ci-scripts/install_host.sh'
                sh './ci-scripts/build_firmware.sh PRALINE'
                script {
                    allOff()
                    reset('hpro_eut')
                }
                sh 'sleep 1s'
                retry(3) {
                    sh './ci-scripts/test_host.sh'
                }
                script {
                    reset('hpro_tester hpro_eut')
                }
                sh 'sleep 1s'
                script {
                    // Allow up to 3 retries 5 minutes each for the HIL test
                    runCommand(3, 5, 'MINUTES', "HackRF Pro Test", hpro_test)
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
    runCommand(3, 20, 'SECONDS', 'USB hub port power server command', "hubs all off")
}

def reset(devices) {
    // Allow up to 3 retries, 20 seconds each, for the USB hub port power server to respond appropriately
    runCommand(3, 20, 'SECONDS', 'USB hub port power server command', "hubs ${devices} reset")
}

def runCommand(retries, time, unit, title, cmd) {
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
