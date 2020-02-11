# SMTPMail-drogon
Simple Mail for the Drogon framework.

It is made as a plugin for the drogon framework.
It can be included into the drogon build with little
modification of the class declaration.

## Appreciations


## Usage
```
...
#include "../plugins/SMTPMail.h"
...

//Inside some function, E.g. A controller function.
...
auto *smtpmailPtr = app().getPlugin<SMTPMail>();
smtpmailPtr->sendEmail(
          "127.0.0.1",                  //The server IP, dns not support by drogon tcp-socket at the moment
          587,                          //The port
          "mailer@something.com",       //Who send the email
          "receiver@otherthing.com",    //Send to whom
          "Testing SMTPMail Function",  //Email Subject/Title
          "Hello from drogon plugin",   //Content
          "mailer@something.com",       //Login user
          "123456"                      //User password
          );
...

```
