import requests

headers = {"Content-Type": "application/json",
                            "Authorization": "key=AIzaSyDHdW5TlcGBwReOe5WhRBoH3aDzRlVmbNU"}

json = '{"notification": {"body" : "Move was detected!","title" : "Check it!"},"to":"/topics/movementDetection",}'

requests.post('https://fcm.googleapis.com/fcm/send', data=json, headers=headers)