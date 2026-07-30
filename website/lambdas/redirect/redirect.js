function handler(event) {
  var request = event.request;
  var uri = request.uri;

  // If URI has a file extension, pass through as-is
  if (uri.match(/\.[a-zA-Z0-9]+$/)) {
    return request;
  }

  // If URI doesn't end with /, redirect to add trailing slash
  if (!uri.endsWith('/')) {
    return {
      statusCode: 301,
      statusDescription: 'Moved Permanently',
      headers: { location: { value: uri + '/' } },
    };
  }

  // URI ends with / — rewrite to index.html
  request.uri = uri + 'index.html';
  return request;
}
