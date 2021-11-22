# maildump-milter

Sendmail-Milter to dump or archive mails on filesystem.
Created to add incoming mails to a mail archive.

Tested with Postfix (1000+ mails per minute)

## Overview

### maildump

To dump all mails from Postfix as message files into a local directory

    +---------+                    +----------+                  +--------------------+
    | Postfix | --smtpd_milters--> | maildump | --save *.msg --> | /var/.../maildump/ |
    +---------+                    +----------+                  +--------------------+


### mailforward

To forward message files to a SMTP-Server

    +-------------------------+           +-------------+           +--------------+
    | /var/.../maildump/*.msg | --read--> | mailforward | --smtp--> | Mail-Archive |
    +-------------------------+           +-------------+           +--------------+
    

    
## Setup

Requirements

    apt install libmilter-dev
    apt install libbsd-dev
    apt install libmailutils-dev
    apt install libmariadb-dev
    apt install curl

Optional:

    apt install catdoc
    apt install html2text

### maildump

To dump all mails from Postfix, a postfix milter must be set up.
For more information about milters see http://www.postfix.org/MILTER_README.html


 #### Create directory

    mkdir /var/spool/postfix/maildump/
    chown postfix /var/spool/postfix/maildump/


 #### Edit `/etc/postfix/main.cf`:

    smtpd_milters = unix:/var/spool/postfix/maildump/maildump.socket
    non_smtpd_milters = unix:/var/spool/postfix/maildump/maildump.socket

*Note:* If Posfix is running in chroot jail, path to socket is `/postfix/maildump/maildump.socket`:

    smtpd_milters = unix:/maildump/maildump.socket
    non_smtpd_milters = unix:/maildump/maildump.socket


#### Test

You can test if the milter is working by launching it as user postfix:
     
    sudo -u postfix ./bin/maildump
    
Now send a message to your Postfix server and check output of command above.
Message files will be stored in `/var/spool/postfix/maildump/` with filename `<timestamp>-<random>.msg`


### mailforward

The `mailforward` program reads a message file and sends it to a SMTP-Server.
No configration required.

Usage: `mailforward <host> <port> <from> <to> <filename> [filename...]`

Parameter     | Description
--------------| -----------
host          | SMTP Hostname or address
port          | SMTP Port
from          | Envelope-From name (used in SMTP-Dialog)
to            | Envelope-Recipient name (used in SMTP-Dialog)
filename      | Path and name of file to be sent
