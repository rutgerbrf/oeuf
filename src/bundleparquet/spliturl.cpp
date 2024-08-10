// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <cstring>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include <curl/curl.h>

#include "spliturl.hpp"

// splitUrl takes a URL of the shape '[http[s]://]HOST[:PORT][/PATH]', and
// splits it into two URLs:
//   - scheme + host -> '[http[s]://]HOST'
//   - port   + path -> '[PORT][/PATH]'
// In case an IPv6 address is provided, the host must be enclosed in square
// brackets. The zone ID may also be indicated. Note that in the resulting
// parts, the colon preceding the port number is omitted. This is on purpose.
std::optional<SplitUrl> splitUrl(const std::string &url, std::string *error) {
  std::stringstream errs;
  std::optional<SplitUrl> result;
  char   *processed      = nullptr;
  char   *scheme         = nullptr;
  char   *user           = nullptr;
  char   *password       = nullptr;
  char   *zoneid         = nullptr;
  char   *query          = nullptr;
  char   *fragment       = nullptr;
  CURLU  *schemehost     = nullptr;
  char   *schemehost_url = nullptr;
  char   *portpath_url   = nullptr;

  // Parse the URL, allowing the user to omit the scheme. CURL will use 'https'
  // by default if no scheme is specified.

  CURLU *parsed = curl_url();
  CURLUcode rc = curl_url_set(parsed, CURLUPART_URL, url.c_str(), CURLU_DEFAULT_SCHEME);
  if (rc != CURLUE_OK) {
    errs << "Failed to parse URL: " << curl_url_strerror(rc);
    goto Exit;
  }

  // As we parse the URL with the option CURLU_DEFAULT_SCHEME, the CURL API
  // won't require the user to provide the scheme part of the URL. It will
  // automatically default the scheme to https. However, we do not usually want
  // it to default to HTTPS, but HTTP instead. (In this specific use case of
  // connecting to a PushGateway server, we assume that the PushGateway server
  // is available over a trusted network, and only using unsecured HTTP).
  // 
  // This is why we check if the scheme was put there by CURL and otherwise set
  // it to HTTP. We also check for any other schemes that the user may have
  // provided, and reject anything that is not http/https.
  if (!url.starts_with("http://") && !url.starts_with("https://")) {
    rc = curl_url_get(parsed, CURLUPART_SCHEME, &scheme, 0);
    if (rc != CURLUE_OK) {
      errs << "Could not get scheme from parsed URL: " << curl_url_strerror(rc);
      goto Exit;
    }
    if (strcmp(scheme, "https")) {
      errs << "Unexpected scheme" << scheme << "in provided URL (expected http or https)";
      goto Exit;
    }
    rc = curl_url_set(parsed, CURLUPART_SCHEME, "http", 0);
    if (rc != CURLUE_OK) {
      errs << "Could not set URL scheme to http: " << curl_url_strerror(rc);
      goto Exit;
    }
  }

  // Turn the parsed URL back into a string.
  rc = curl_url_get(parsed, CURLUPART_URL, &processed, 0);
  if (rc != CURLUE_OK) {
    errs << "Failed to output parsed URL: " << curl_url_strerror(rc);
    goto Exit;
  }

  // This part of the code checks if no prohibited parts are present in the URL
  // (basic auth: (user, password), query, fragment).

  rc = curl_url_get(parsed, CURLUPART_USER, &user, 0);
  if (rc == CURLUE_OK && strlen(user) != 0) {
    errs << "Provided URL should not contain a user part";
    goto Exit;
  } else if (rc != CURLUE_NO_USER && rc != CURLUE_OK) {
    errs << "Failed to get check user part existence in provided url: " << curl_url_strerror(rc);
    goto Exit;
  }

  rc = curl_url_get(parsed, CURLUPART_PASSWORD, &password, 0);
  if (rc == CURLUE_OK && strlen(password) != 0) {
    errs << "Provided URL should not contain a password part";
    goto Exit;
  } else if (rc != CURLUE_NO_PASSWORD && rc != CURLUE_OK) {
    errs << "Failed to get check password part existence in provided url: " << curl_url_strerror(rc);
    goto Exit;
  }

  rc = curl_url_get(parsed, CURLUPART_QUERY, &query, 0);
  if (rc == CURLUE_OK && strlen(query) != 0) {
    errs << "Provided URL should not contain a query part";
    goto Exit;
  } else if (rc != CURLUE_NO_QUERY && rc != CURLUE_OK) {
    errs << "Failed to get check query part existence in provided url: " << curl_url_strerror(rc);
    goto Exit;
  }

  rc = curl_url_get(parsed, CURLUPART_FRAGMENT, &fragment, 0);
  if (rc == CURLUE_OK && strlen(fragment) != 0) {
    errs << "Provided URL should not contain a fragment part";
    goto Exit;
  } else if (rc != CURLUE_NO_FRAGMENT && rc != CURLUE_OK) {
    errs << "Failed to get check fragment part existence in provided url: " << curl_url_strerror(rc);
    goto Exit;
  }

  // Now that we know that the provided URL makes sense, we can start doing
  // some arts and crafts. We get started by copying the parsed URL into
  // schemehost and simply delete all parts which are not scheme + host.

  schemehost = curl_url_dup(parsed);

  rc = curl_url_set(schemehost, CURLUPART_PORT, nullptr, 0);
  if (rc != CURLUE_OK) {
    errs << "Could not unset port in duplicated URL: " << curl_url_strerror(rc);
    goto Exit;
  }
  rc = curl_url_set(schemehost, CURLUPART_PATH, nullptr, 0);
  if (rc != CURLUE_OK) {
    errs << "Could not unset path in duplicated URL: " << curl_url_strerror(rc);
    goto Exit;
  }

  // Okay, now we have the schemehost CURLU all ready to go. Note that a URL
  // only consisting of a scheme and host is considered valid, so CURL will be
  // more than happy to actually turn it into a string for us. Which is exactly
  // what we do here :)

  rc = curl_url_get(schemehost, CURLUPART_URL, &schemehost_url, 0);
  if (rc != CURLUE_OK) {
    errs << "Could not get scheme + host URL: " << curl_url_strerror(rc);
    goto Exit;
  }

  // Remove any trailing slash after the scheme + host URL that CURL might have
  // put there -- we still want to get a valid URL if we paste the port + path
  // part behind it.

  if (strlen(schemehost_url) > 0) {
    if (schemehost_url[strlen(schemehost_url) - 1] != '/') {
      errs << "Scheme + host URL does not end with a slash";
      goto Exit;
    }
    schemehost_url[strlen(schemehost_url) - 1] = '\0';
  }

  // Look, this is really gross. Because the port + path part of the URL is not
  // a valid URL itself, but the scheme + host should be a prefix of the full
  // URL containing the port + path, we can simply check if it is indeed a
  // prefix, and then strip it from the full URL, giving us the port + path
  // (after deleting the colon preceding the port).

  if (!std::string_view(processed).starts_with(schemehost_url)) {
    errs << "Scheme + host URL is not a prefix of the processed URL";
    goto Exit;
  }

  portpath_url = processed + strlen(schemehost_url);
  // We should not have the colon before the port, prometheus-cpp inserts it
  if (strlen(portpath_url) > 0 && portpath_url[0] == ':') portpath_url++;
  // We do not need a trailing slash
  if (strlen(portpath_url) > 0 && portpath_url[strlen(portpath_url)-1] == '/')
    portpath_url[strlen(portpath_url)-1] = '\0';

  // It has been done. BLECH
  result = std::make_optional<SplitUrl>(schemehost_url, portpath_url);

Exit:
  curl_free(processed);
  curl_free(scheme);
  curl_free(user);
  curl_free(password);
  curl_free(query);
  curl_free(fragment);
  curl_free(zoneid);
  curl_free(schemehost_url);
  curl_url_cleanup(schemehost);
  curl_url_cleanup(parsed);

  if (!result && error)
    *error = errs.str();

  return result;
}
