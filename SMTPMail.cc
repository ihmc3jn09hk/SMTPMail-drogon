/** 
* 
* SMTPMail.cc *
* 
* This plugin is for SMTP mail delievery for the Drogon web-framework. Implementation 
* reference from the project "SMTPClient" with Qt5 by kelvins. Please check out
* https://github.com/kelvins/SMTPClient.

Feel free to use the code. For the sake of any concern, the following licence is attached.

 Copyright 2020 ihmc3jn09hk

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#include "SMTPMail.h"
#include <trantor/net/TcpClient.h>
#include <trantor/net/EventLoopThread.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/utils/Utilities.h>

using namespace drogon;
using namespace trantor;

struct EMail
{
    enum states{
        Init,
        HandShake,
        Tls,
        Auth,
        User,
        Pass,
        Mail,
        Rcpt,
        Data,
        Body,
        Quit,
        Close
    };
    std::string m_from;
    std::string m_to;
    std::string m_subject;
    std::string m_content;
    std::string m_user;
    std::string m_passwd;
    states m_status;
    std::string m_uuid;
    std::shared_ptr<trantor::TcpClient> m_socket;
    
    EMail(  const std::string &from,
            const std::string &to,
            const std::string &subject,
            const std::string &content,
            const std::string &user,
            const std::string &passwd,
            std::shared_ptr<trantor::TcpClient> socket)
            :m_from(from), 
            m_to(to),
            m_subject(subject),
            m_content(content),
            m_user(user),
            m_passwd(passwd),
            m_socket(socket),
            m_uuid(utils::getUuid())
        {
            m_status = Init;
        }
        
    ~EMail(){
    }
    
    static std::unordered_map<std::string, std::shared_ptr<EMail>> m_emails;    //Container for processing emails
};

std::unordered_map<std::string, std::shared_ptr<EMail>> EMail::m_emails;

void SMTPMail::initAndStart(const Json::Value &config)
{
    /// Initialize and start the plugin
    LOG_TRACE << "SMTPMail initialized";
}

void SMTPMail::shutdown() 
{
    /// Shutdown the plugin
    LOG_TRACE << "STMPMail Shutdown";
}

void messagesHandle( const trantor::TcpConnectionPtr &connPtr,
    trantor::MsgBuffer *msg,
    std::shared_ptr<EMail> email,
    const std::function<void(const std::string &msg)> &cb )
{  
    auto msgSize = msg->readableBytes();
    std::string receievedMsg;
    while (msg->readableBytes() > 0)
    {
        std::string buf(msg->peek(), msg->readableBytes());
        receievedMsg.append( buf );
//        LOG_INFO << buf;
        msg->retrieveAll();
    }
    std::string responseCode(receievedMsg.begin(), receievedMsg.begin() + 3);
//    std::string responseMsg(receievedMsg.begin() + 4, receievedMsg.end());

    if ( email->m_status == EMail::Init && responseCode == "220" )
    {
        std::string outMsg;
        trantor::MsgBuffer out;
        
        outMsg.append("EHLO ¡Hola!");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::HandShake;
    }
    else if ( email->m_status == EMail::HandShake && responseCode == "220" )
    {
        std::string outMsg;
        trantor::MsgBuffer out;
        
        outMsg.append("EHLO ¡Hola!");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());
        
        connPtr->startClientEncryption([connPtr, out]() {
                        //LOG_TRACE << "SSL established";
                        connPtr->send(std::move(out));
                    });

        email->m_status = EMail::Auth;
    }
    else if (email->m_status == EMail::HandShake && responseCode == "250")
    {
        std::string outMsg;
        trantor::MsgBuffer out;
        
        outMsg.append("EHLO ¡Hola!");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::Tls;
    }
    else if (email->m_status == EMail::Tls && responseCode == "250")
    {

        std::string outMsg;
        trantor::MsgBuffer out;
        
        outMsg.append("STARTTLS");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::HandShake;
    }
    else if (email->m_status == EMail::Auth && responseCode == "250")
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        outMsg.append("AUTH LOGIN");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::User;
    }
    else if (email->m_status == EMail::User && responseCode == "334")
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        std::string screte(email->m_user);

        //outMsg.append(base64_encode(reinterpret_cast<const unsigned char*>(screte.c_str()), screte.length()));
        outMsg.append(utils::base64Encode(reinterpret_cast<const unsigned char*>(screte.c_str()), screte.length()));
        
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));
        
        email->m_status = EMail::Pass;
    }
    else if (email->m_status == EMail::Pass && responseCode == "334")
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        std::string screte(email->m_passwd);

        outMsg.append(utils::base64Encode(reinterpret_cast<const unsigned char*>(screte.c_str()), screte.length()));
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::Mail;
    }
    else if ( email->m_status == EMail::Mail && responseCode == "235" )
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        outMsg.append("MAIL FROM:<");
        outMsg.append(email->m_from);
        outMsg.append(">\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));
        
        email->m_status = EMail::Rcpt;
    }
    else if ( email->m_status == EMail::Rcpt && responseCode == "250" )
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        outMsg.append("RCPT TO:<");
        outMsg.append(email->m_to);
        outMsg.append(">\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::Data;
    }
    else if ( email->m_status == EMail::Data && responseCode == "250" )
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        outMsg.append("DATA");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::Body;
    }
    else if ( email->m_status == EMail::Body && responseCode == "354" )
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        outMsg.append("To: " + email->m_to + "\r\n");
        outMsg.append("From: " + email->m_from + "\r\n");
        outMsg.append("Subject: " + email->m_subject + "\r\n");
        outMsg.append(email->m_content + "\r\n");
        outMsg.append("\r\n.\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));
        
        email->m_status = EMail::Quit;
    }
    else if ( email->m_status == EMail::Quit && responseCode == "250" )
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        outMsg.append("QUIT");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));
        
        email->m_status = EMail::Close;
    }
    else if ( email->m_status == EMail::Close )
    {
        /*Callback here for succeed delievery is propable*/
        cb( "EMail sent. ID : " + email->m_uuid );
        return;
    }
    else
    {
        email->m_status = EMail::Close;
        /*Callback here for notification is propable*/
        cb( receievedMsg );
    }
}

std::string  SMTPMail::sendEmail( const std::string &mailServer,  
    const uint16_t &port,
    const std::string &from,
    const std::string &to,
    const std::string &subject,
    const std::string &content,
    const std::string &user,
    const std::string &passwd,
    const std::function<void(const std::string&)> &cb 
    )
{
    if ( mailServer.empty() || from.empty() || to.empty() || subject.empty() || user.empty() || passwd.empty()){
        LOG_WARN << "Invalid input(s) - "
                << "\nServer : " << mailServer
                << "\nPort : " << port
                << "\nfrom : " << from
                << "\nto : " << to
                << "\nsubject : " << subject
                << "\nuser : " << user
                << "\npasswd : " << passwd;
        return std::string();
    }
    LOG_TRACE << "New TcpClient : " << mailServer << ":" << port;

    //auto loop = app().getLoop();//m_thread->getLoop();
    auto loop = app().getIOLoop(10); //Get the IO Loop
    assert(loop);                   //Should never be null

    auto tcpSocket =
            std::make_shared<trantor::TcpClient>(loop, InetAddress( mailServer, port, false ), "SMTPMail");

    //Create the email
    auto email = std::make_shared<EMail>(from, to, subject, content, user, passwd, tcpSocket);
    std::weak_ptr<EMail> email_wptr = email;

    EMail::m_emails.emplace( email->m_uuid, email ); //Assuming there is no uuid collision
    tcpSocket->setConnectionCallback(
        [email_wptr](const trantor::TcpConnectionPtr &connPtr) {
            auto email_ptr = email_wptr.lock();
            if ( !email_ptr ) {
                LOG_WARN << "EMail pointer gone";
                return;
            }
            if (connPtr->connected()) {
                // send request;
                LOG_TRACE << "Connection established!";
            } else {
                LOG_TRACE << "Connection disconnect";
                EMail::m_emails.erase(email_ptr->m_uuid);   //Remove the email in list
                //thisPtr->onError(std::string("ReqResult::NetworkFailure"));
            }
        });
    tcpSocket->setConnectionErrorCallback(
      [email_wptr]() {
        auto email_ptr = email_wptr.lock();
        if ( !email_ptr ) {
            LOG_ERROR << "EMail pointer gone";
            return;
        }
        // can't connect to server
        LOG_ERROR << "Bad Server address";
        EMail::m_emails.erase(email_ptr->m_uuid);   //Remove the email in list
        //thisPtr->onError(std::string("ReqResult::BadServerAddress"));
    });
    auto cb_( cb? cb : [](const std::string &msg){ LOG_INFO << "Default email callback : " << msg;});
    tcpSocket->setMessageCallback(
        [email_wptr, cb_](const trantor::TcpConnectionPtr &connPtr,
                  trantor::MsgBuffer *msg) {
            auto email_ptr = email_wptr.lock();
            if ( !email_ptr ) {
                LOG_ERROR << "EMail pointer gone";
                return;
            }
            //email->m_socket->disconnect();
            messagesHandle(connPtr, msg, email_ptr, cb_);
        });
    tcpSocket->connect(); //Start trying to send the email
    return email->m_uuid;
}
