# Server main configuration file
This server is designed to be compiled once and reconfigured. Most options can be reconfigured with a simple server restart. The configuration file is in an xml(-like) format. Below is the documentation for all configuration file field values.


The default location for the configuration file is "configuration.xml" in the working directory of the server.
To override the default location for the configuration file, define `CONFIGURATION_FILE_LOCATION` as a __C String__, as a compile option (`-DCONFIGURATION_FILE_LOCATION="{whatever}"`), or edit the default value in `/config.h`


## Database

```xml
<database>
        <settings>
                <maximum_concurrent_connections>{number connections [int]}</maximum_concurrent_connections>
        </settings>
        <authentication>
                <database_name>{database name}</database_name>
                <host>{database host}</host>
                <port>{database port [int]}
                <username>{database username}</username>
                <password>{database password}</password>
        </authentication>
</database>
```

- [int] database > settings > maximum_concurrent_connections
  - The maximum number of connections the database will allow at once.
  - If this value is exceeded, the connection manager will attempt a new connection every 100 milliseconds, configurable by adjusting MAXCONNECTIONSPOLLWAIT as a server compile-time argument
  - 0 __(not recommended)__ disables limit
- [string] database > authentication > database_name
  - The name of the PostgreSQL database used by the application
- [string] database > authentication > host
  - The hostname for the database server
  - Recommended default: localhost
- [int] database > authentication > port
  - The port on which the database server is listening
  - PostgreSQL: Recommended default: 5432
- [string] database > authentication > username
  - The username used to authenticate into the database
- [string] database > authentication > password
  - The password used to authenticate into the database


## HTTP Server settings

```xml
<http>
        <request>
                <request_line>
                        <maximum_length>{max first line bytes before error[int]}</maximum_length>
                </requestline>
                <headers>
                        <maximum_count>{num [int]}</maximum_count>
                        <maximum_key_length>{num [int]}</maximum_key_length>
                        <maximum_value_length>{num [int]}</maximum_value_length>
                </headers>
        </request>
</http>
```

- [int] http > request > request_line > maximum_length
  - Maximum number of bytes to be downloaded from the client, stating the method, path and protocol
  - Recommended: 1042
  - 0 __(not recommended)__ disables limit
- [int] http > request > headers > maximum_count
  - Maximum number of headers to be downloaded from client before sending back an error
  - Recommended: 64
  - 0 __(not recommended)__ disables limit
- [int] http > request > headers > maximum_key_length
  - Maximum length of a header key to be downloaded from a client before sending back an error
  - Recommended: 128
  - 0 __(not recommended)__ disables limit
- [int] http > request > headers > maximum_value_length
  - Maximum length of a header value that can be downloaded from a client before sending back an error
  - 0 __(not recommended)__ disables limit
