
Text Mining API
---------------
Text Mining API is a simple pluggable framework module that handle node/taxonomy terms association process for tagging services.


Requirements
------------
http://drupal.org/project/process_api


Proxy support
-------------
There is a quick proxy support for intranet sites using PEAR::HTTP_Request2.
You have to specify crendential in settings :
$conf['textmining_api_proxy_host'] = 'proxy.example.com';
$conf['textmining_api_proxy_port'] = '8080';
$conf['textmining_api_proxy_user'] = 'user';
$conf['textmining_api_proxy_password'] = '*******';


Plugin
------
Plugins are module providing a Process API services with the main class extending TextminingApiServiceAbstract.
You have to implement th function TextminingApiServiceInterface::threadGenerateItemTerms().
See contrib/textmining_random dummy module.


Contrib modules
---------------
- OpenCalais
- Tagthe.net


Quick Start
-----------
- Go in the Process API settings.
- Set up a server based on your specific term service.
- Set up a thread based on the new server. You have to choose the taxonomy_reference field to populate. You have to choose at least one field that will provide text data.