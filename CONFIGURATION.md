# Server main configuration file
This server is designed to be compiled once and reconfigured
Most options can be reconfigured with a simple server restart
---

### The default location for the configuration file is "configuration.xml" in the working directory of the server.
To override the default location for the configuration file, define CONFIGURATION_FILE_LOCATION as a C String, as a compile option, or edit the default value in "/config.h"

---

## Database

```xml
<database>
        <settings>
                <maxconcurrentconnections>{number connections [int]}</maxconcurrentconnections>
        </settings>
        <authentication>
                <dbname>{database name}</dbname>
                <host>{database host}</host>
                <port>{database port [int]}
                <username>{database username}</username>
                <password>{database password}</password>
        </authentication>
</database>
```

- database > settings > maxconcurrentconnections
  - The maximum number of connections the database will allow at once.
  - If this value is exceeded, the connection manager will attempt a new connection every 100 milliseconds, configurable by adjusting MAXCONNECTIONSPOLLWAIT as a server compile-time argument
  - 0 __(not recommended)__ disables limit
- database > authentication > dbname
  - The name of the PostgreSQL database used by the application
- database > authentication > host
  - The hostname for the database server
  - Recommended default: localhost
- database > authentication > port
  - The port on which the database server is listening
  - PostgreSQL: Recommended default: 5432
- database > authentication > username
  - The username used to authenticate into the database
- database > authentication > password
  - The password used to authenticate into the database

---

## HTTP Server settings

```xml
<http>
        <request>
                <firstline>
                        <maximumlength>{max first line bytes before error[int]}</maxlength>
                </firstline>
                <headers>
                        <maxheaders>{num [int]}</maxheaders>
                        <maxheaderkeylength>{num [int]}</maxheaderkeylength>
                        <maxheadervaluelength>{num [int]}</maxheadervaluelength>
                </headers>
        </request>
</http>
```

- http > request > firstline > maximumlength
  - Maximum number of bytes to be downloaded from the client, stating the method, path and protocol
  - Recommended: 1042
  - 0 __(not recommended)__ disables limit
- http > request > headers > maxheaders
  - Maximum number of headers to be downloaded from client before sending back an error
  - Recommended: 64
  - 0 __(not recommended)__ disables limit
- http > request > headers > maxheaderkeylength
  - Maximum length of a header key to be downloaded from a client before sending back an error
  - Recommended: 128
  - 0 __(not recommended)__ disables limit
- http > request > headers > maxheadervaluelength
  - Maximum length of a header value that can be downloaded from a client before sending back an error
  - 0 __(not recommended)__ disables limit
