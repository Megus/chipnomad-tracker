exports.handler = (event, context, callback) => {
    const request = event.Records[0].cf.request;
    const uri = request.uri;

    // Match any '/' that occurs at the end of a URI. Replace it with a default index
    var newuri = uri.replace(/\/$/, '\/index.html');
    request.uri = newuri;

    return callback(null, request);
};