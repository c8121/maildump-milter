# maildump-milter

Sendmail-Milter to dump or archive mails on filesystem.
Created to add incoming mails to a mail archive.

Tested with Postfix (1000+ mails per minute)

## maildump

    +---------+                    +----------+                  +--------------------+
    | Postfix | --smtpd_milters--> | maildump | --save *.msg --> | /var/.../maildump/ |
    +---------+                    +----------+                  +--------------------+
                                                                     
## mailforward

    +-------------------------+           +-------------+           +--------------+
    | /var/.../maildump/*.msg | --read--> | mailforward | --smtp--> | Mail-Archive |
    +-------------------------+           +-------------+           +--------------+
    


