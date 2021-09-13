# SMTPMail-drogon
Simple Mail for the Drogon framework.

It is made as a plugin for the [drogon](https://github.com/an-tao/drogon) framework.
It can be included into the drogon build with little 
modification of the class declaration.
## Update [13-09-2021] Add support HTML content

Added [HTML content support](https://github.com/ihmc3jn09hk/SMTPMail-drogon/pull/1).

## Update [23-12-2020] Add DNS support

Added DNS support. 

## Appreciations
* The implementation takes SMTPClient for Qt from [kelvins](https://github.com/kelvins/SMTPClient) as reference.
* There requires a delay SSL encryption from the Tcp-socket (named TcpClient in trantor/drogon) and the major 
author of drogon [reponsed](https://github.com/an-tao/drogon/issues/346) quickly. 

## Usage
Download to the plugin directory of the target drogon app, E.g. ~/drogon-app/plugins
```bash
$ git clone https://github.com/ihmc3jn09hk/SMTPMail-drogon.git
$ cp SMTPMail-drogon/SMTPMail.* ~/drogon-app/plugins
```

* _Be aware of add the plugin into the config.json. Set the "name" field to "SMTPMail"_

Add the reference header and get the plugin from the app(), E.g. 

```c++
...
#include "../plugins/SMTPMail.h"
...

//Inside some function, E.g. A controller function.
...
//Send an email
auto *smtpmailPtr = app().getPlugin<SMTPMail>();
auto id = smtpmailPtr->sendEmail(
          "127.0.0.1",                  //The server IP/DNS
          587,                          //The port
          "mailer@something.com",       //Who send the email
          "receiver@otherthing.com",    //Send to whom
          "Testing SMTPMail Function",  //Email Subject/Title
          "Hello from drogon plugin",   //Content
          "mailer@something.com",       //Login user
          "123456"                      //User password
          );
...
//Or get noted when email is sent
...
void callback(const std::string &msg)
{ 
  LOG_INFO << msg; /*Output e.g. "EMail sent. ID : 96ESERVDDFH17588ECF0C7B00326E3"*/
  /*Do whatever you like*/
}
...
auto *smtpmailPtr = app().getPlugin<SMTPMail>();
auto id = smtpmailPtr->sendEmail(
          "127.0.0.1",                  //The server IP/DNS
          587,                          //The port
          "mailer@something.com",       //Who send the email
          "receiver@otherthing.com",    //Send to whom
          "Testing SMTPMail Function",  //Email Subject/Title
          "Hello from drogon plugin",   //Content
          "mailer@something.com",       //Login user
          "123456",                     //User password
          callback                      //Callback
          );
```

```bash
$ cd ~/drogon-app/build
$ make
```

## Licence
* Feel free to use, thanks to open-source.
* For the sake of concern on commercial usage, a simple licence is included in each of the files.
