import urllib.request
import hashlib
password=""
f = urllib.request.urlopen("http://59.167.158.119/status")
data = f.read()
token = str(data).split("\"")[-2]  #dodgy expects token to be in an exact place
a = token + password + "output45"
hashed = hashlib.md5(a.encode('utf-8')).hexdigest()
f = urllib.request.urlopen("http://59.167.158.119/t/" + hashed + "/output/45")
a = token + password + "schedu30045D45"
hashed = hashlib.md5(a.encode('utf-8')).hexdigest()
f = urllib.request.urlopen("http://59.167.158.119/t/" + hashed + "/schedu/30/0/45/D/45")

#http://59.167.158.119/t/f5c88163ad3cf8a61b44f75c921193b7/output/45
#http://59.167.158.119/t/42148877f65d4e726240c18cd863dae1/schedu/12/0/45/D/45
