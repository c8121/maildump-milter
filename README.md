# maildump-milter

Sendmail-Milter to dump or archive mails on filesystem.
Created to add incoming mails to a mail archive.

Tested with Postfix (1000+ mails per minute)

## Overview

### maildump

    +---------+                    +----------+                  +--------------------+
    | Postfix | --smtpd_milters--> | maildump | --save *.msg --> | /var/.../maildump/ |
    +---------+                    +----------+                  +--------------------+
                                                                     
### mailforward

    +-------------------------+           +-------------+           +--------------+
    | /var/.../maildump/*.msg | --read--> | mailforward | --smtp--> | Mail-Archive |
    +-------------------------+           +-------------+           +--------------+
    

    
## Setup

### maildump

To dump all mails from Postfix


 1.  Create directory

    `mkdir /var/spool/postfix/maildump/`

    `chown postfix /var/spool/postfix/maildump/`


 2. Edit `/etc/postfix/main.cf`:

    `smtpd_milters = unix:/var/spool/postfix/maildump/maildump.socket`

    *Note:* If Posfix is running in chroot jail, path to socket is `/postfix/maildump/maildump.socket`:

    `smtpd_milters = unix:/maildump/maildump.socket`


You can test if the milter is working by launching it as user postfix:

    `sudo -u postfix ./bin/maildump`


