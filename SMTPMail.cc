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
#include "base64/base64.h"
#include <trantor/net/TcpClient.h>
#include <trantor/net/EventLoopThread.h>
#include <drogon/HttpAppFramework.h>
#include <string>

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
            m_socket(socket){ m_status = Init; }
};

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
    std::shared_ptr<EMail> email)
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
        
        outMsg.append("EHLO localhost");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        email->m_status = EMail::HandShake;
    }
    else if ( email->m_status == EMail::HandShake && responseCode == "220" )
    {
        LOG_WARN << "Enabling SSL";
        
        std::string outMsg;
        trantor::MsgBuffer out;
        
        outMsg.append("EHLO localhost");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());
        
        connPtr->startClientEncryption([connPtr, out]() {
                        LOG_INFO << "SSL established";
                        connPtr->send(std::move(out));
                    });

        email->m_status = EMail::Auth;
    }
    else if (email->m_status == EMail::HandShake && responseCode == "250")
    {
        std::string outMsg;
        trantor::MsgBuffer out;
        
        outMsg.append("EHLO localhost");
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));
        
        LOG_WARN << "Next to STARTTLS";

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
        
        LOG_WARN << "Next to HandShake";

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
        
        LOG_WARN << "Next to User ID";

        email->m_status = EMail::User;
    }
    else if (email->m_status == EMail::User && responseCode == "334")
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        std::string screte(email->m_user);

        outMsg.append(base64_encode(reinterpret_cast<const unsigned char*>(screte.c_str()), screte.length()));
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));

        LOG_WARN << "Next to Password";
        
        email->m_status = EMail::Pass;
    }
    else if (email->m_status == EMail::Pass && responseCode == "334")
    {
        trantor::MsgBuffer out;
        std::string outMsg;
        
        std::string screte(email->m_passwd);

        outMsg.append(base64_encode(reinterpret_cast<const unsigned char*>(screte.c_str()), screte.length()));
        outMsg.append("\r\n");
        
        out.append( outMsg.data(), outMsg.size());

        connPtr->send(std::move(out));
        
        LOG_WARN << "Next to Sender info";

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

         /*Callback here for succeed delievery is propable*/
    }
    else if ( email->m_status == EMail::Close )
    {
        email->m_socket->disconnect();
        return;
    }
    else
    {
        email->m_status = EMail::Close;
        /*Callback here for notification is propable*/
    }
}

bool SMTPMail::sendEmail( const std::string &mailServer,  
    const uint16_t &port,
    const std::string &from,
    const std::string &to,
    const std::string &subject,
    const std::string &content,
    const std::string &user,
    const std::string &passwd )
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
        return false;
    }
    LOG_INFO << "Ready to send email";
    LOG_TRACE << "New TcpClient : " << mailServer << ":" << port;
    
    //auto loop = app().getLoop();//m_thread->getLoop();
    auto loop = app().getIOLoop(10); //Get the IO Loop
    assert(loop);                   //Should never be null
    
    auto tcpSocket =
            std::make_shared<trantor::TcpClient>(loop, InetAddress( mailServer, port, false ), "SMTPMail");
    
    //Create the email
    auto email = std::make_shared<EMail>(from, to, subject, content, user, passwd, tcpSocket);
    
    tcpSocket->setConnectionCallback(
        [](const trantor::TcpConnectionPtr &socketPtr) {
            if (socketPtr->connected())
            {
                // send request;
                LOG_TRACE << "Connection established!";
            }
            else
            {
                LOG_TRACE << "connection disconnect";
                //thisPtr->onError(std::string("ReqResult::NetworkFailure"));
            }
        });
    tcpSocket->setConnectionErrorCallback(
      []() {
        // can't connect to server
        LOG_ERROR << "Bad Server address";
        //thisPtr->onError(std::string("ReqResult::BadServerAddress"));
    });
    tcpSocket->setMessageCallback(
        [email](const trantor::TcpConnectionPtr &socketPtr,
                  trantor::MsgBuffer *msg) {
            messagesHandle(socketPtr, msg, email);
        });
    tcpSocket->connect(); //Start trying to send the email
    return true;
}
