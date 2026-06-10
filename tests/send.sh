#!/bin/zsh
../tools/sshpass -p '0000' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -q -p 22 root@mac270 \
  'kextstat | grep -q KextRW || kextload /Users/seo/Desktop/KextRW/KextRW.kext';
../tools/sshpass -p '0000' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -q -p 22 seo@mac270 'rm ~/kextrw_test';
../tools/sshpass -p '0000' scp -q -r -ostricthostkeychecking=false -ouserknownhostsfile=/dev/null -o StrictHostKeyChecking=no -P 22 ../build/bin/kextrw_test 'seo@mac270:~/kextrw_test';
../tools/sshpass -p '0000' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -q -p 22 seo@mac270 'sync; sync; sync; sync; sync; sync; sync; sync; sync;';