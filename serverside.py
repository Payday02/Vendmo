#!/usr/bin/env python3
"""
 * Project: Vendmo
 * File name: serverside.py
 * Description:  Email Parsing to Blynk
 *
 * @author Paden Davis
 * @email pdavis77@gatech.edu
 *
 """
import imaplib
import requests
import time

ttl = 0
file = open("pw.txt", "r")
(server, port, username, password, auth_token, emailverif, emailauth) = file.read().splitlines()

print("\nConnecting to server",server,"on port",port,"and logging in as",username)
mail = imaplib.IMAP4_SSL(server, port)
typ, data = mail.login(username, password)
print(typ)
mail.select('INBOX')

while 1:
    ttl += 1;
    time.sleep(5)

    try:
        print(ttl, end=',', flush=True)
        mail.select('INBOX')
        typ,data = mail.search(None,'UNSEEN SUBJECT "paid you"')
        data = data[0].decode('UTF-8').split(" ")
        count = len(data)
        if (data == ['']):
            count = 0

        if (count):
            for i in range(count):
                print("\n\nFetching message",i,"of",count,"...")
                typ,msgdata = mail.fetch(data[i], '(BODY.PEEK[HEADER])')
                mail.store(data[i],'+FLAGS','\\SEEN')
                msgdata = msgdata[0][1].decode('UTF-8').splitlines()
                linecount = 0
                for line in msgdata:
                    linecount += 1
                    if (line.find("paid you") != -1):
                        subject = line.split(" ",1)[1]
                    elif (line.find("Date: ") != -1):
                        timestamp = line[6:-6]
                    elif (line.find(emailverif)  != -1):
                        authentication = msgdata[linecount].split("=",1)[1].split(" ",1)[0]

                try:
                    if (authentication != emailauth):
                        print("Unverified Transaction")
                        print("Transaction:",subject)
                        print("Timestamp:",timestamp)
                        continue
                    print("Message Authentication: VERIFIED")
                    print("Transaction:",subject)
                    print("Timestamp:",timestamp)

                    reqmsg = timestamp + " > " + subject

                    URL = "http://blynk-cloud.com/"+auth_token+"/update/V9?value="+reqmsg
                    print("Dispensing Transaction...\n")
                    r = requests.get(url = URL)
                    mail.store(data[i],'+FLAGS','\\Deleted')
                    time.sleep(5)

                except:
                    print("This shit aint gucci\n")
                    continue
    except:
        print("***EXCEPTION***\n")
        print("\nConnecting to server ", server, " on port ", port, " and logging in as ", username)
        mail = imaplib.IMAP4_SSL(server, port)
        typ, data = mail.login(username, password)
        print(typ)
        mail.select('INBOX')
