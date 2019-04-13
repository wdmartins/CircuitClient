/*
 * CircuitClient.h - Library for Circuit platform
 * Created by Walter D. Martins, March 21, 2019
 */
#ifndef CIRCUIT_CLIENT_H
#define CIRCUIT_CLIENT_H

#include "Arduino.h"
#include <CircuitConfig.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

using namespace std;

#define DEBUG_CIRCUIT_CLIENT
#define DEBUG_OUTPUT Serial

namespace circuit {
  /*------------------------------------------------------------------------------------------*/
  /* Constants Definitions                                                                    */
  /*------------------------------------------------------------------------------------------*/
  
  /* Circuit Domains                                                                          */
  const string kSandBoxUrl = "circuitsandbox.net";
  const string kSandBoxFingerprint = "E2:D8:96:33:4F:F7:A3:66:AF:EF:A2:04:11:9C:39:D8:D6:DD:DA:95";

  /* Rest API Urls                                                                            */
  const string kRestApiVersionUrl = "/rest/v2";
  const string kConvEndpointUrl = "/conversations/";
  const string kMessagesEndpointUrl =  "/messages";
  const string kCircuitWebHooksUrl = "/webhooks";
  const string kUserProfileEndpointUrl = "/users/profile";
  const string kUserPresenceEndpointUrl = "/users/presence";
  const string kUserPresenceWebhookUrl = "/presence";

  /* Other Constants                                                                          */
  const int kUUIDLength = 61;

  /* User Presence States                                                                     */
  const string kPresenceBusy = "BUSY";
  const string kPresenceAvailable = "AVAILABLE";
  const string kPresenceAway = "AWAY";
  const string kPresenceDnd = "DND";

  class UrlBuilder {
    public:
      UrlBuilder(string domain, string protocol = "https", string restApiVersion = kRestApiVersionUrl);

      // https:://circuitsandbox.net/rest/v2/webhooks
      string getWebHooksUrl(void);
      // https:://circuitsandbox.net/rest/v2/webhooks/presence
      string getPresenceWebHooksUrl(void);
      // https:://circuitsandbox.net/rest/v2/conversations/{convId}/messages
      string getMessagesUrl(string convId);
      // https:://circuitsandbox.net/rest/v2/users/profile
      string getUserProfileUrl();
      // https:://circuitsandbox.net/rest/v2/users/presence
      string getUserPresenceUrl(string userId);

    protected:
      string _baseUrl;

    protected:
      // https:://circuitsandbox.net/rest/v2/conversations/{convId}
      string _getConversationUrl(string convId);
  };

  class HttpWrapper {
    public:
      HttpWrapper(bool debug = false);
      HttpWrapper(string b64EncodedCredentials, bool debug = false);
      HttpWrapper(string username, string password, bool debug = false);

      void setCredentials(string b64EncodedCredentials);
      void setCredentials(string username, string password);
      void setServerFingerprint(string fingerPrint);
      void setHttpClient(HTTPClient *http);
      void setDebug(void);
      void resetDebug(void);
      string getPayload();
      int POST(string url, string content);
      int GET(string url);
      int DELETE(string url);
      int PUT(string url, string content);

    protected:
      HTTPClient *_http;
      string _credentials;
      string _serverFingerprint;
      bool _debugEnabled;
      string _lastPayload;

  };
  /*------------------------------------------------------------------------------------------*/
  /* Circuit Client                                                                           */
  /*------------------------------------------------------------------------------------------*/
  class CircuitClient {
    public:
      CircuitClient(string domain, string credentials, string convId);
      CircuitClient(string credentials, string convId);
      
      void init(void);
      void setConversationId(string convId);
      int postTextMessage(string text);
      void setOnNewTextItemCallBack( void (*func)(string) );
      const char *getUserPresence(char* userId);
      void setOnUserPresenceChange(char* userId, void (*func)(string) );
      void run();

    protected:
      HttpWrapper *_http;
      void (*_onNewTextItemCB)(string);
      void (*_onUserPresenceChangeCB)(string);
      UrlBuilder *_urlBuilder;
      string _domain;
      string _credentials;
      char _userId[kUUIDLength];
      string _convId;
      bool _server_started;
      void _deleteAllWebHooks(void);
      void _getAllWebhooks(void);
      void _getUserProfile(void);
      void _handleNewTextItem(void);
      void _handleNotFound(void);
      void _handleUserPresenceChange(void);
      void _init(string domain, string credentials, string convId);
      void _startServer(void);
  };

}

#endif //CIRCUIT_CLIENT_H
