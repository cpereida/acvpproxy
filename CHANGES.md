v0.7.0
- fix SHAKE support
- add SHAKE definitions for OpenSSL
- add AES-GCM-SIV support
- add GET / POST /persons
- add GET / PUT /persons/<personID>
- add PUT /vendors/<vendorID>
- add PUT /modules/<moduleID>
- add PUT /dependencies/<dependencyID>
- add PUT /oes/<dependencyID>
- add Paging support
- add bugfix when writing JSON files
- ACVP v1.0 support: change URLs and versions
- ACVP add revision support
- make /large support working after it is enabled on ACVP server
- add support for range domain values defined with DEF_ALG_DOMAIN
- add tests verifying the generation of requests

v0.6.5
- add /large endpoint handling
- speed up --cipher-options
- use pthread mutexes
- remove message queue always during startup to prevent attaching to a stale
  message queue and restarting the MQ server election process
- Fix support for --resubmit-results

Changes v0.6.4
- MQ server test: fix for enabling testing on macOS
- add HMAC-SHA3 and SHA3 definitions for OpenSSL

Changes v0.6.3
- fix curl code compile issue on old libcurl versions
- MQ server: use busy-wait around client-side msgrcv to ensure catching a signal
- add test cases

Changes 0.6.2
- enable safety check guaranteeing that module definition did not change between test vector request and test response submission
- fix bug in acvp_publish
- SIGSEGV is caught to remove message queue
- OpenSSL: add missing CBC
- statically link JSON-C
- fix make cppcheck
- compile with -Wno-missing-field-initializers as some compilers require all structure fields to be initialized
- re-enable ACVP cancel operation upon SIGTERM, SIGINT, SIGQUIT, SIGHUP
- allow ACVP cancel operation to be terminated with 2nd receipt of signals

Changes 0.6.1
- Support --vsid without --testid on command line
- MQ server: fix starting and restarting of server thread
- fix IKEv2 register operation

Changes 0.6.0
 * first public release with support for ACVP v0.5