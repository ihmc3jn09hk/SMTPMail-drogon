/**
 *
 *  SMTPMail.h
 *
 * This plugin is for SMTP mail delievery for the Drogon web-framework.
Implementation
 * reference from the project "SMTPClient" with Qt5 by kelvins. Please check out
 * https://github.com/kelvins/SMTPClient.

Copyright 2020 ihmc3jn09hk

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once

#include <drogon/plugins/Plugin.h>

class SMTPMail : public drogon::Plugin<SMTPMail> {
public:
  SMTPMail() = default;
  /// This method must be called by drogon to initialize and start the plugin.
  /// It must be implemented by the user.
  void initAndStart(const Json::Value &config) override;

  /// This method must be called by drogon to shutdown the plugin.
  /// It must be implemented by the user.
  void shutdown() override;

  /** Send an email
   * return : An ID of the email.
   */
  std::string sendEmail(
      const std::string
          &mailServer, // Mail server address/dns E.g. 127.0.0.1/smtp.mail.com
      const uint16_t &port,       // Port  E.g. 587
      const std::string &from,    // Send from whom E.g. drogon@gmail.com
      const std::string &to,      // Reciever       E.g. drogon@yahoo.com
      const std::string &subject, // The email title/subject
      const std::string &content, // The email content.
      const std::string &user,    // User      (Usually same as "from")
      const std::string &passwd,  // Password
      bool isHTML,                // content type
      const std::function<void(const std::string &)> &cb = {}
      // The callback for email sent notification
  );
};
