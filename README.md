# maildump-milter

Sendmail-Milter to dump or archive mails on filesystem.
Created to add incoming mails to a mail archive.

Barely tested with postfix right now (2021-05-25). 

## maildump

    +---------+                    +----------+                  +--------------------+
    | Postfix | --smtpd_milters--> | maildump | --save *.msg --> | /var/.../maildump/ |
    +---------+                    +----------+                  +--------------------+
                                                                     
## mailforward

    +-------------------------+           +--------------+
    | /var/.../maildump/*.msg | --smtp--> | Mail-Archive |
    +-------------------------+           +--------------+
    


