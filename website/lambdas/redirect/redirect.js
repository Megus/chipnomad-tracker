exports.handler = (event, context, callback) => {
    const request = event.Records[0].cf.request;
    const uri = request.uri;
    
    // Check whether the URI is missing a file extension.
    if (!uri.includes('.')) {
        request.uri += '/index.html';
    }
    
    callback(null, request);
};