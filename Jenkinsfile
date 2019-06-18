node ('ami') {
    stage ('git'){
        git branch: 'develop', url: 'https://github.com/Remmeauth/remchain.git'
        sh 'git submodule update --init --recursive'
    }
    stage("build"){
        sh '''
        ls /
        #wget -q https://github.com/EOSIO/eosio.cdt/releases/download/v1.6.1/eosio.cdt_1.6.1-1_amd64.deb
        #sudo apt install ./eosio.cdt_1.6.1-1_amd64.deb
        #./scripts/eosio_build.sh -y REM -m
        '''
    }
    telegramSend 'Hello World'
}
