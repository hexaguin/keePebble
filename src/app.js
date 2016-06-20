console.log('keePebble JS loaded');

Pebble.addEventListener('appmessage', function(e) {
  // Get the dictionary from the message
//   var dict = e.payload;
//   var dictObj = JSON.parse(dict);
  console.log('JS Got message: ' + JSON.stringify(e.payload));
  console.log('Body is ' + e.payload[2]);
  //if (dictObj["0"]=="PLACEHOLDER PLEASE DO THE PROTOCOL STUFF"){
    //TODO stuff
  //}
});