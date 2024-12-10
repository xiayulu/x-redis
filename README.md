# Build Your Own Redis

Chanllenge Overview: https://app.codecrafters.io/courses/redis/overview

a golang solution: https://github.com/ValentinJub/Redis-codecrafters

从0开始，手写Redis: https://www.cnblogs.com/crazymakercircle/p/17532100.html

Build Your Own Redis with C/C++ : https://build-your-own.org/redis/

## SetUp Environment

### Setup official redis server

Install Redis:

```shell
sudo pacman -S redis
```

The Redis configuration file is well-documented and located at `/etc/redis/redis.conf`

```shell
port 6379
```

Start/Enable Redis:

```shell
sudo systemctl start redis.service
```

Connect redis server by `redis-cli`:

```shell
$ redis-cli -p 6666
127.0.0.1:6666> ping
PONG
```

### Setup C++ dev environment

- git
- gcc, clang
- cmake
- vcpkg
- VSCode: clangd, cmake tools(cmake, clangd-cmake).

https://github.com/microsoft/vcpkg

https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash

```shell
export VCPKG_ROOT="$HOME/.local/share/vcpkg"
export PATH="$PATH:$VCPKG_ROOT"

git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT
cd $VCPKG_ROOT
# update repository
git pull

# The bootstrap script performs prerequisite checks and downloads the vcpkg executable.
bootstrap-vcpkg.sh -disableMetrics
```

### Run Demo projects

```shell
git clone <demo>

vcpkg install
./your_program.sh
```

## implement support for the [PING](https://redis.io/commands/ping) command.

Redis clients communicate with Redis servers by sending "[commands](https://redis.io/commands/)". For each command, a Redis server sends a response back to the client. Commands and responses are both encoded using the [Redis protocol](https://redis.io/topics/protocol) (we'll learn more about this in later stages).

[PING](https://redis.io/commands/ping/) is one of the simplest Redis commands. It's used to check whether a Redis server is healthy.

The response for the `PING` command is `+PONG\r\n`. This is the string "PONG" encoded using the [Redis protocol](https://redis.io/docs/reference/protocol-spec/).

In this stage, we'll cut corners by ignoring client input and hardcoding `+PONG\r\n` as a response. We'll learn to parse client input in later stages.

### Notes

- You can ignore the data that the tester sends you for this stage. We'll get to parsing client input in later stages. For now, you can just hardcode `+PONG\r\n` as the response.
- You can also ignore handling multiple clients and handling multiple  PING commands in the stage, we'll get to that in later stages.
- The exact bytes your program will receive won't be just `PING`, you'll receive something like this: `*1\r\n$4\r\nPING\r\n`, which is the Redis protocol encoding of the `PING` command. We'll learn more about this in later stages.

```c++
  // Accept a connection from a client
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                         (socklen_t *)&client_addr_len);
  if (client_fd < 0) {
    std::cerr << "accept failed\n";
    return 1;
  }

  std::cout << "Client connected\n";

  if (send(client_fd, "+PONG\r\n", 7, 0) < 0) {

    std::cerr << "send failed\n";
    return 1;
  }

  // close
  close(server_fd);
  close(client_fd);
```

## Respond to multiple PINGs

In this stage, you'll respond to multiple [PING](https://redis.io/commands/ping) commands sent by the same connection.

A Redis server starts to listen for the next command as soon as it's done responding to the previous one. This allows Redis clients to send multiple commands using the same connection.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then send two PING commands using the same connection:

```bash
$ echo -e "PING\nPING" | redis-cli
```

The tester will expect to receive two `+PONG\r\n` responses.

You'll need to run a loop that reads input from a connection and sends a response back.

### Notes

- Just like the previous stage, you can hardcode `+PONG\r\n` as the response for this stage. We'll get to parsing client input in later stages.
- The two PING commands will be sent using the same connection. We'll get to handling multiple connections in later stages.

## Handle concurrent clients

In this stage, you'll add support for multiple concurrent clients.

In addition to handling multiple commands from the same client, Redis servers are also designed to handle multiple clients at once.

To implement this, you'll need to either use threads, or, if you're feeling adventurous, an [Event Loop](https://en.wikipedia.org/wiki/Event_loop) (like the official Redis implementation does).

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then send two PING commands concurrently using two different connections:

```bash
# These two will be sent concurrently so that we test your server's ability to handle concurrent clients.
$ redis-cli PING
$ redis-cli PING
```

The tester will expect to receive two `+PONG\r\n` responses.

### Notes

- Since the tester client *only* sends the PING command at the moment, it's okay to ignore what the client sends and hardcode a response. We'll get to parsing client input in later stages.

### Resources

The best explanation for event loop I've seen is [this talk by Philip Robert](https://www.youtube.com/watch?v=8aGhZQkoFbQ&ab_channel=JSConf) — *What the heck is the event loop anyway?*

## Implement the ECHO command

In this stage, you'll add support for the [ECHO](https://redis.io/commands/echo) command.

`ECHO` is a command like `PING` that's used for testing and debugging. It accepts a single argument and returns it back as a RESP bulk string.

```bash
$ redis-cli PING # The command you implemented in previous stages
PONG
$ redis-cli ECHO hey # The command you'll implement in this stage
hey
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then send an `ECHO` command with an argument to your server:

```bash
$ redis-cli ECHO hey
```

The tester will expect to receive `$3\r\nhey\r\n` as a response (that's the string `hey` encoded as a [RESP bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings).

### Notes

- We suggest that you implement a proper Redis protocol parser in this stage. It'll come in handy in later stages.
- Redis command names are case-insensitive, so `ECHO`, `echo` and `EcHo` are all valid commands.
- The tester will send a random string as an argument to the `ECHO` command, so you won't be able to hardcode the response to pass this stage.
- The exact bytes your program will receive won't be just `ECHO hey`, you'll receive something like this: `*2\r\n$4\r\nECHO\r\n$3\r\nhey\r\n`. That's `["ECHO", "hey"]` encoded using the [Redis protocol](https://redis.io/docs/reference/protocol-spec/).
- You can read more about how "commands" are handled in the Redis protocol [here](https://redis.io/docs/reference/protocol-spec/#sending-commands-to-a-redis-server).

## Implement the SET & GET commands

### Your Task

In this stage, you'll add support for the [SET](https://redis.io/commands/set) & [GET](https://redis.io/commands/get) commands.

The `SET` command is used to set a key to a value. The `GET` command is used to retrieve the value of a key.

```bash
$ redis-cli SET foo bar
OK
$ redis-cli GET foo
bar
```

The `SET` command supports a number of extra options like `EX` (expiry time in seconds), `PX` (expiry time in milliseconds) and more. We won't cover these extra options in this stage. We'll get to them in later stages.

### Tests

The tester will execute your program like this:

```bash
./your_program.sh
```

It'll then send a `SET` command to your server:

```bash
$ redis-cli SET foo bar
```

The tester will expect to receive `+OK\r\n` as a response (that's the string `OK` encoded as a [RESP simple string](https://redis.io/docs/reference/protocol-spec/#resp-simple-strings)).

This command will be followed by a `GET` command:

```bash
$ redis-cli GET foo
```

The tester will expect to receive `$3\r\nbar\r\n` as a response (that's the string `bar` encoded as a [RESP bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings).

### Notes

- If you implemented a proper Redis protocol parser in the previous stage, you should be able to reuse it in this stage.
- Just like the previous stage, the values used for keys and values  will be random, so you won't be able to hardcode the response to pass  this stage.
- If a key doesn't exist, the `GET` command should return a "null bulk string" (`$-1\r\n`). We won't explicitly test this in this stage, but you'll need it for the next stage (expiry).

## Expiry

### Your Task

In this stage, you'll add support for setting a key with an expiry.

The expiry for a key can be provided using the "PX" argument to the [SET](https://redis.io/commands/set) command. The expiry is provided in milliseconds.

```bash
$ redis-cli SET foo bar px 100 # Sets the key "foo" to "bar" with an expiry of 100 milliseconds
OK
```

After the key has expired, a `GET` command for that key should return a "null bulk string" (`$-1\r\n`).

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then send a `SET` command to your server to set a key with an expiry:

```bash
$ redis-cli SET foo bar px 100
```

It'll then immediately send a `GET` command to retrieve the value:

```bash
$ redis-cli GET foo
```

It'll expect the response to be `bar` (encoded as a RESP bulk string).

It'll then wait for the key to expire and send another `GET` command:

```bash
$ sleep 0.2 && redis-cli GET foo
```

It'll expect the response to be `$-1\r\n` (a "null bulk string").

### Notes

- Just like command names, command arguments are also case-insensitive. So `PX`, `px` and `pX` are all valid.
- The keys, values and expiry times used in the tests will be random,  so you won't be able to hardcode a response to pass this stage.

## RDB file config

### Your Task

Welcome to the RDB Persistence Extension! In this extension, you'll add support for reading [RDB files](https://redis.io/docs/management/persistence/) (Redis Database files).

In this stage, you'll add support for two configuration parameters related to RDB persistence, as well as the [CONFIG GET](https://redis.io/docs/latest/commands/config-get/) command.

### RDB files

An RDB file is a point-in-time snapshot of a Redis dataset. When RDB  persistence is enabled, the Redis server syncs its in-memory state with  an RDB file, by doing the following:

1. On startup, the Redis server loads the data from the RDB file.
2. While running, the Redis server periodically takes new snapshots of the dataset, in order to update the RDB file.

### `dir` and `dbfilename`

The configuration parameters `dir` and `dbfilename` specify where an RDB file is stored:

- `dir` - the path to the directory where the RDB file is stored (example: `/tmp/redis-data`)
- `dbfilename` - the name of the RDB file (example: `rdbfile`)

### The `CONFIG GET` command

The [`CONFIG GET`](https://redis.io/docs/latest/commands/config-get/) command returns the values of configuration parameters.

It takes in one or more configuration parameters and returns a [RESP array](https://redis.io/docs/latest/develop/reference/protocol-spec/#arrays) of key-value pairs:

```bash
$ redis-cli CONFIG GET dir
1) "dir"
2) "/tmp/redis-data"
```

Although `CONFIG GET` can fetch multiple parameters at a time, the tester will only send `CONFIG GET` commands with one parameter at a time.

### Tests

The tester will execute your program like this:

```bash
./your_program.sh --dir /tmp/redis-files --dbfilename dump.rdb
```

It'll then send the following commands:

```bash
$ redis-cli CONFIG GET dir
$ redis-cli CONFIG GET dbfilename
```

Your server must respond to each `CONFIG GET` command with a RESP array containing two elements:

1. The parameter **name**, encoded as a [RESP Bulk string](https://redis.io/docs/latest/develop/reference/protocol-spec/#bulk-strings)
2. The parameter **value**, encoded as a RESP Bulk string

For example, if the value of `dir` is `/tmp/redis-files`, then the expected response to `CONFIG GET dir` is:

```bash
*2\r\n$3\r\ndir\r\n$16\r\n/tmp/redis-files\r\n
```

### Notes

- You don't need to read the RDB file in this stage, you only need to store `dir` and `dbfilename`. Reading from the file will be covered in later stages.
- If your repository was created before 5th Oct 2023, it's possible that your `./your_program.sh` script is not passing arguments to your program. To fix this, you'll need to edit `./your_program.sh`. Check the [update CLI args PR](https://github.com/codecrafters-io/build-your-own-redis/pull/89/files) for details on how to do this.

## Read a key

### Your Task

In this stage, you'll add support for reading a single key from an RDB file.

### RDB file format

#### RDB file format overview

Here are the different sections of the RDB file, in order:

1. Header section
2. Metadata section
3. Database section
4. End of file section

RDB files use special encodings to store different types of data. The ones relevant to this stage are "size encoding" and "string encoding."  These are explained near the end of this page.

The following breakdown of the RDB file format is based on [Redis RDB File Format](https://rdb.fnordig.de/file_format.html) by Jan-Erik Rediger. We’ve only included the parts that are relevant to this stage.

#### Header section

RDB files begin with a header section, which looks something like this:

```
52 45 44 49 53 30 30 31 31  // Magic string + version number (ASCII): "REDIS0011".
```

The header contains the magic string `REDIS`, followed by a four-character RDB version number. In this challenge, the test RDB  files all use version 11. So, the header is always `REDIS0011`.

#### Metadata section

Next is the metadata section. It contains zero or more "metadata  subsections," which each specify a single metadata attribute. Here's an  example of a metadata subsection that specifies `redis-ver`:

```
FA                             // Indicates the start of a metadata subsection.
09 72 65 64 69 73 2D 76 65 72  // The name of the metadata attribute (string encoded): "redis-ver".
06 36 2E 30 2E 31 36           // The value of the metadata attribute (string encoded): "6.0.16".
```

The metadata name and value are always string encoded.

#### Database section

Next is the database section. It contains zero or more "database  subsections," which each describe a single database. Here's an example  of a database subsection:

```
FE                       // Indicates the start of a database subsection.
00                       /* The index of the database (size encoded).
                            Here, the index is 0. */

FB                       // Indicates that hash table size information follows.
03                       /* The size of the hash table that stores the keys and values (size encoded).
                            Here, the total key-value hash table size is 3. */
02                       /* The size of the hash table that stores the expires of the keys (size encoded).
                            Here, the number of keys with an expiry is 2. */
00                       /* The 1-byte flag that specifies the value’s type and encoding.
                            Here, the flag is 0, which means "string." */
06 66 6F 6F 62 61 72     // The name of the key (string encoded). Here, it's "foobar".
06 62 61 7A 71 75 78     // The value (string encoded). Here, it's "bazqux".
FC                       /* Indicates that this key ("foo") has an expire,
                            and that the expire timestamp is expressed in milliseconds. */
15 72 E7 07 8F 01 00 00  /* The expire timestamp, expressed in Unix time,
                            stored as an 8-byte unsigned long, in little-endian (read right-to-left).
                            Here, the expire timestamp is 1713824559637. */
00                       // Value type is string.
03 66 6F 6F              // Key name is "foo".
03 62 61 72              // Value is "bar".
FD                       /* Indicates that this key ("baz") has an expire,
                            and that the expire timestamp is expressed in seconds. */
52 ED 2A 66              /* The expire timestamp, expressed in Unix time,
                            stored as an 4-byte unsigned integer, in little-endian (read right-to-left).
                            Here, the expire timestamp is 1714089298. */
00                       // Value type is string.
03 62 61 7A              // Key name is "baz".
03 71 75 78              // Value is "qux".
```

Here's a more formal description of how each key-value pair is stored:

1. Optional expire information (one of the following):
   - Timestamp in seconds:
     1. `FD`
     2. Expire timestamp in seconds (4-byte unsigned integer)
   - Timestamp in milliseconds:
     1. `FC`
     2. Expire timestamp in milliseconds (8-byte unsigned long)
2. Value type (1-byte flag)
3. Key (string encoded)
4. Value (encoding depends on value type)

#### End of file section

This section marks the end of the file. It looks something like this:

```
FF                       /* Indicates that the file is ending,
                            and that the checksum follows. */
89 3b b7 4e f8 0f 77 19  // An 8-byte CRC64 checksum of the entire file.
```

#### Size encoding

Size-encoded values specify the size of something. Here are some examples:

- The database indexes and hash table sizes are size encoded.
- String encoding begins with a size-encoded value that specifies the number of characters in the string.
- List encoding begins with a size-encoded value that specifies the number of elements in the list.

The first two bits of a size-encoded value indicate how the value  should be parsed. Here's a guide (bits are shown in both hexadecimal and binary):

```
/* If the first two bits are 0b00:
   The size is the remaining 6 bits of the byte.
   In this example, the size is 10: */
0A
00001010

/* If the first two bits are 0b01:
   The size is the next 14 bits
   (remaining 6 bits in the first byte, combined with the next byte),
   in big-endian (read left-to-right).
   In this example, the size is 700: */
42 BC
01000010 10111100

/* If the first two bits are 0b10:
   Ignore the remaining 6 bits of the first byte.
   The size is the next 4 bytes, in big-endian (read left-to-right).
   In this example, the size is 17000: */
80 00 00 42 68
10000000 00000000 00000000 01000010 01101000

/* If the first two bits are 0b11:
   The remaining 6 bits specify a type of string encoding.
   See string encoding section. */
```

#### String encoding

A string-encoded value consists of two parts:

1. The size of the string (size encoded).
2. The string.

Here's an example:

```
/* The 0x0D size specifies that the string is 13 characters long.
   The remaining characters spell out "Hello, World!". */
0D 48 65 6C 6C 6F 2C 20 57 6F 72 6C 64 21
```

For sizes that begin with `0b11`, the remaining 6 bits indicate a type of string format:

```
/* The 0xC0 size indicates the string is an 8-bit integer.
   In this example, the string is "123". */
C0 7B

/* The 0xC1 size indicates the string is a 16-bit integer.
   The remaining bytes are in little-endian (read right-to-left).
   In this example, the string is "12345". */
C1 39 30

/* The 0xC2 size indicates the string is a 32-bit integer.
   The remaining bytes are in little-endian (read right-to-left),
   In this example, the string is "1234567". */
C2 87 D6 12 00

/* The 0xC3 size indicates that the string is compressed with the LZF algorithm.
   You will not encounter LZF-compressed strings in this challenge. */
C3 ...
```

### The `KEYS` command

The [`KEYS command`](https://redis.io/docs/latest/commands/keys/) returns all the keys that match a given pattern, as a RESP array:

```
$ redis-cli SET foo bar
OK
$ redis-cli SET baz qux
OK
$ redis-cli KEYS "f*"
1) "foo"
```

When the pattern is `*`, the command returns all the keys in the database:

```
$ redis-cli KEYS "*"
1) "baz"
2) "foo"
```

In this stage, you must add support for the `KEYS` command. However, you only need to support the `*` pattern.

### Tests

The tester will create an RDB file with a single key and execute your program like this:

```
$ ./your_program.sh --dir <dir> --dbfilename <filename>
```

It'll then send a `KEYS "*"` command to your server.

```
$ redis-cli KEYS "*"
```

Your server must respond with a RESP array that contains the key from the RDB file:

```
*1\r\n$3\r\nfoo\r\n
```

### Notes

- The RDB file provided by `--dir` and `--dbfilename` might not exist. If the file doesn't exist, your program must treat the database as empty.
- RDB files use both little-endian and big-endian to store numbers. See the [MDN article on endianness](https://developer.mozilla.org/en-US/docs/Glossary/Endianness) to learn more.
- To generate an RDB file, use the [`SAVE` command](https://redis.io/docs/latest/commands/save/).

## Read a string value

### Your Task

In this stage, you'll add support for reading the value corresponding to a key from an RDB file.

Just like with the previous stage, we'll stick to supporting RDB files that contain a single key for now.

The tester will create an RDB file with a single key and execute your program like this:

```
./your_program.sh --dir <dir> --dbfilename <filename>
```

It'll then send a `GET <key>` command to your server.

```bash
$ redis-cli GET "foo"
```

The response to `GET <key>` should be a RESP bulk string with the value of the key.

For example, let's say the RDB file contains a key called `foo` with the value `bar`. The expected response will be `$3\r\nbar\r\n`.

Strings can be encoded in three different ways in the RDB file format:

- Length-prefixed strings
- Integers as strings
- Compressed strings

In this stage, you only need to support length-prefixed strings. We won't cover the other two types in this challenge.

We recommend using [this blog post](https://rdb.fnordig.de/file_format.html) as a reference when working on this stage.

## Read multiple keys

### Your Task

In this stage, you'll add support for reading multiple keys from an RDB file.

The tester will create an RDB file with multiple keys and execute your program like this:

```bash
$ ./your_program.sh --dir <dir> --dbfilename <filename>
```

It'll then send a `KEYS *` command to your server.

```bash
$ redis-cli KEYS "*"
```

The response to `KEYS *` should be a RESP array with the keys as elements.

For example, let's say the RDB file contains two keys: `foo` and `bar`. The expected response will be:

```
*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n
```

- `*2\r\n` indicates that the array has two elements
- `$3\r\nfoo\r\n` indicates that the first element is a bulk string with the value `foo`
- `$3\r\nbar\r\n` indicates that the second element is a bulk string with the value `bar`

## Read multiple string values

### Your Task

In this stage, you'll add support for reading multiple string values from an RDB file.

The tester will create an RDB file with multiple keys and execute your program like this:

```bash
$ ./your_program.sh --dir <dir> --dbfilename <filename>
```

It'll then send multiple `GET <key>` commands to your server.

```bash
$ redis-cli GET "foo"
$ redis-cli GET "bar"
```

The response to each `GET <key>` command should be a RESP bulk string with the value corresponding to the key.

## Read value with expiry

### Your Task

In this stage, you'll add support for reading values that have an expiry set.

The tester will create an RDB file with multiple keys. Some of these  keys will have an expiry set, and some won't. The expiry timestamps will also be random, some will be in the past and some will be in the  future.

The tester will execute your program like this:

```bash
$ ./your_program.sh --dir <dir> --dbfilename <filename>
```

It'll then send multiple `GET <key>` commands to your server.

```bash
$ redis-cli GET "foo"
$ redis-cli GET "bar"
```

When a key has expired, the expected response is `$-1\r\n` (a "null bulk string").

When a key hasn't expired, the expected response is a RESP bulk string with the value corresponding to the key.

## Configure listening port

### Your Task

Welcome to the Replication extension!

In this extension, you'll extend your Redis server to support [leader-follower replication](https://redis.io/docs/management/replication/). You'll be able to run multiple Redis servers with one acting as the "master" and the others as "replicas". Changes made to the master will be automatically replicated to replicas.

Since we'll need to run multiple instances of your Redis server at once, we can't run all of them on port 6379.

In this stage, you'll add support for starting the Redis server on a  custom port. The port number will be passed to your program via the `--port` flag.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port 6380
```

It'll then try to connect to your TCP server on the specified port number (`6380` in the example above). If the connection succeeds, you'll pass this stage.

### Notes

- Your program still needs to pass the previous stages, so if `--port` isn't specified, you should default to port 6379.
- The tester will pass a random port number to your program, so you can't hardcode the port number from the example above.
- If your repository was created before 5th Oct 2023, it's possible that your `./your_program.sh` script might not be passing arguments on to your program. You'll need to edit `./your_program.sh` to fix this, check [this PR](https://github.com/codecrafters-io/build-your-own-redis/pull/89/files) for details.

## The INFO command

### Your Task

In this stage, you'll add support for the [INFO](https://redis.io/commands/info/) command.

The `INFO` command returns information and statistics about a Redis server. In this stage, we'll add support for the `replication` section of the `INFO` command.

### The replication section

When you run the `INFO` command against a Redis server, you'll see something like this:

```
$ redis-cli INFO replication
# Replication
role:master
connected_slaves:0
master_replid:8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb
master_repl_offset:0
second_repl_offset:-1
repl_backlog_active:0
repl_backlog_size:1048576
repl_backlog_first_byte_offset:0
repl_backlog_histlen:
```

The reply to this command is a [Bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings) where each line is a key value pair, separated by ":".

Here are what some of the important fields mean:

- `role`: The role of the server (`master` or `slave`)
- `connected_slaves`: The number of connected replicas
- `master_replid`: The replication ID of the master (we'll get to this in later stages)
- `master_repl_offset`: The replication offset of the master (we'll get to this in later stages)

In this stage, you'll only need to support the `role` key. We'll add support for other keys in later stages.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT>
```

It'll then send the `INFO` command with `replication` as an argument.

```bash
$ redis-cli -p <PORT> info replication
```

Your program should respond with a [Bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings) where each line is a key value pair separated by `:`. The tester will only look for the `role` key, and assert that the value is `master`.

### Notes

- In the response for the `INFO` command, you only need to support the `role` key for this stage. We'll add support for the other keys in later stages.

- The `# Replication` heading in the response is optional, you can ignore it.

- The response to `INFO` needs to be encoded as a [Bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings).

  - An example valid response would be `$11\r\nrole:master\r\n` (the string `role:master` encoded as a [Bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings))

- The `INFO` command can be used without any arguments, in which case it returns all sections available. In this stage, we'll always send `replication` as an argument to the `INFO` command, so you only need to support the `replication` section.

## The INFO command on a replica

### Your Task

In this stage, you'll extend your [INFO](https://redis.io/commands/info/) command to run on a replica.

### The `--replicaof` flag

By default, a Redis server assumes the "master" role. When the `--replicaof` flag is passed, the server assumes the "slave" role instead.

Here's an example usage of the `--replicaof` flag:

```
./your_program.sh --port 6380 --replicaof "localhost 6379"
```

In this example, we're starting a Redis server in replica mode. The  server itself will listen for connections on port 6380, but it'll also connect to a master (another Redis server) running on localhost  port 6379 and replicate all changes from the master.

We'll learn more about how this replication works in later stages. For now, we'll focus on adding support for the `--replicaof` flag, and extending the `INFO` command to support returning `role: slave` when the server is a replica.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT> --replicaof "<MASTER_HOST> <MASTER_PORT>"
```

It'll then send the `INFO` command with `replication` as an argument to your server.

```bash
$ redis-cli -p <PORT> info replication
```

Your program should respond with a [Bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings) where each line is a key value pair separated by `:`. The tester will only look for the `role` key, and assert that the value is `slave`.

### Notes

- Your program still needs to pass the previous stage tests, so if `--replicaof` isn't specified, you should default to the `master` role.
- Just like the last stage, you only need to support the `role` key in the response for this stage. We'll add support for the other keys in later stages.
- You don't need to actually connect to the master server specified via `--replicaof` in this stage. We'll get to that in later stages.

## Initial Replication ID and Offset

### Your Task

In this stage, you'll extend your `INFO` command to return two additional values: `master_replid` and `master_repl_offset`.

### The replication ID and offset

Every Redis master has a replication ID: it is a large pseudo random string. This is set when the master is booted. Every time a master instance restarts from scratch, its replication ID is reset.

Each master also maintains a "replication offset" corresponding to how many bytes of commands have been added to the replication stream. We'll learn more about this offset in later stages. For now, just know that the value starts from `0` when a master is booted and no replicas have connected yet.

In this stage, you'll initialize a replication ID and offset for your master:

- The ID can be any pseudo random alphanumeric string of 40 characters.
  - For the purposes of this challenge, you don't need to actually generate a random string, you can hardcode it instead.
  - As an example, you can hardcode `8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb` as the replication ID.
- The offset is to be 0.

These two values should be returned as part of the INFO command output, under the `master_replid` and `master_repl_offset` keys respectively.

### Tests

The tester will execute your program like this:

```
./your_program.sh
```

It'll then send the `INFO` command with `replication` as an argument to your server.

```bash
$ redis-cli INFO replication
```

Your program should respond with a [Bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings) where each line is a key value pair separated by `:`. The tester will look for the following keys:

- `master_replid`, which should be a 40 character alphanumeric string
- `master_repl_offset`, which should be `0`

### Notes

- Your code should still pass the previous stage tests, so the `role` key still needs to be returned

## Send handshake (1/3)

### Your Task

In this stage, you'll implement part 1 of the handshake that happens when a replica connects to master.

### Handshake

When a replica connects to a master, it needs to go through a handshake process before receiving updates from the master.

There are three parts to this handshake:

- The replica sends a `PING` to the master (**This stage**)
- The replica sends `REPLCONF` twice to the master (Next stages)
- The replica sends `PSYNC` to the master (Next stages)

We'll learn more about `REPLCONF` and `PSYNC` in later stages. For now, we'll focus on the first part of the handshake: sending `PING` to the master.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT> --replicaof "<MASTER_HOST> <MASTER_PORT>"
```

It'll then assert that the replica connects to the master and sends the `PING` command.

### Notes

- The `PING` command should be sent as a RESP Array, like this : `*1\r\n$4\r\nPING\r\n`

## Send handshake (2/3)

### Your Task

In this stage, you'll implement part 2 of the handshake that happens when a replica connects to master.

### Handshake (continued from previous stage)

As a recap, there are three parts to the handshake:

- The replica sends a `PING` to the master (Previous stage)
- The replica sends `REPLCONF` twice to the master (**This stage**)
- The replica sends `PSYNC` to the master (Next stage)

After receiving a response to `PING`, the replica then sends 2 [REPLCONF](https://redis.io/commands/replconf/) commands to the master.

The `REPLCONF` command is used to configure replication. Replicas will send this command to the master twice:

- The first time, it'll be sent like this: 

  ```
  REPLCONF listening-port <PORT>
  ```

  - This is the replica notifying the master of the port it's listening on

- The second time, it'll be sent like this: 

  ```
  REPLCONF capa psync2
  ```

  - This is the replica notifying the master of its capabilities ("capa" is short for "capabilities")
  - You can safely hardcode these capabilities for now, we won't need to use them in this challenge.

These commands should be sent as RESP Arrays, so the exact bytes will look something like this:

```
# REPLCONF listening-port <PORT>
*3\r\n$8\r\nREPLCONF\r\n$14\r\nlistening-port\r\n$4\r\n6380\r\n

# REPLCONF capa psync2
*3\r\n$8\r\nREPLCONF\r\n$4\r\ncapa\r\n$6\r\npsync2\r\n
```

For both commands, the master will respond with `+OK\r\n` ("OK" encoded as a RESP Simple String).

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT> --replicaof "<MASTER_HOST> <MASTER_PORT>"
```

It'll then assert that the replica connects to the master and:

- **(a)** sends the `PING` command
- **(b)** sends the `REPLCONF` command with `listening-port` and `<PORT>` as arguments
- **(c)** sends the `REPLCONF` command with `capa psync2` as arguments

**Notes**

- The response to `REPLCONF` will always be `+OK\r\n` ("OK" encoded as a RESP Simple String)

## Send handshake (3/3)

### Your Task

In this stage, you'll implement part 3 of the handshake that happens when a replica connects to master.

### Handshake (continued from previous stage)

As a recap, there are three parts to the handshake:

- The replica sends a `PING` to the master (Previous stages)
- The replica sends `REPLCONF` twice to the master (Previous stages)
- The replica sends `PSYNC` to the master (**This stage**)

After receiving a response to the second `REPLCONF`, the replica then sends a [PSYNC](https://redis.io/commands/psync/) command to the master.

The `PSYNC` command is used to synchronize the state of  the replica with the master. The replica will send this command to the  master with two arguments:

- The first argument is the replication ID of the master
  - Since this is the first time the replica is connecting to the master, the replication ID will be `?` (a question mark)
- The second argument is the offset of the master
  - Since this is the first time the replica is connecting to the master, the offset will be `-1`

So the final command sent will be `PSYNC ? -1`.

This should be sent as a RESP Array, so the exact bytes will look something like this:

```
*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n
```

The master will respond with a [Simple string](https://redis.io/docs/reference/protocol-spec/#simple-strings) that looks like this:

```
+FULLRESYNC <REPL_ID> 0\r\n
```

You can ignore the response for now, we'll get to handling it in later stages.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT> --replicaof "<MASTER_HOST> <MASTER_PORT>"
```

It'll then assert that the replica connects to the master and:

- **(a)** sends `PING` command
- **(b)** sends `REPLCONF listening-port <PORT>`
- **(c)** sends `REPLCONF capa eof capa psync2`
- **(d)** sends `PSYNC ? -1`

## Receive handshake (1/2)

### Your Task

In this stage, we'll start implementing support for receiving a replication handshake as a master.

### Handshake (continued from previous stage)

We'll now implement the same handshake we did in the previous stages, but on the master instead of the replica.

As a recap, there are three parts to the handshake:

- The master receives a 

  ```
  PING
  ```

   from the replica

  - Your Redis server already supports the `PING` command, so there's no additional work to do here

- The master receives `REPLCONF` twice from the replica (**This stage**)

- The master receives `PSYNC` from the replica (Next stage)

In this stage, you'll add support for receiving the `REPLCONF` command from the replica.

You'll receive `REPLCONF` twice from the replica. For the purposes of this challenge, you can safely ignore the arguments for both commands and just respond with `+OK\r\n` ("OK" encoded as a RESP Simple String).

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT>
```

It'll then send the following commands:

1. `PING` (expecting `+PONG\r\n` back)
2. `REPLCONF listening-port <PORT>` (expecting `+OK\r\n` back)
3. `REPLCONF capa eof capa psync2` (expecting `+OK\r\n` back)

## Receive handshake (2/2)

### Your Task

In this stage, you'll add support for receiving the [`PSYNC`](https://redis.io/commands/psync/) command from the replica.

### Handshake (continued from previous stage)

As a recap, there are three parts to the handshake:

- The master receives a `PING` from the replica (You've already implemented this)
- The master receives `REPLCONF` twice from the replica (You've already implemented this)
- The master receives `PSYNC` from the replica (**This stage**)

After the replica sends `REPLCONF` twice, it'll send a `PSYNC ? -1` command to the master.

- The first argument is `?`
  - This is the replication ID of the master, it is `?` because this is the first time the replica is connecting to the master.
- The second argument is `-1`
  - This is the replication offset, it is `-1` because this is the first time the replica is connecting to the master.

The final command you receive will look something like this:

```
*3\r\n$5\r\nPSYNC\r\n$1\r\n?\r\n$2\r\n-1\r\n
```

(That's `["PSYNC", "?", "-1"]` encoded as a RESP Array)

The master needs to respond with `+FULLRESYNC <REPL_ID> 0\r\n` ("FULLRESYNC  0" encoded as a RESP Simple String). Here's what the response means:

- `FULLRESYNC` means that the master cannot perform incremental replication with the replica, and will thus start a "full" resynchronization.
- `<REPL_ID>` is the replication ID of the master. You've already set this in the "Replication ID & Offset" stage.
  - As an example, you can hardcode `8371b4fb1155b71f4a04d3e1bc3e18c4a990aeeb` as the replication ID.
- `0` is the replication offset of the master. You've already set this in the "Replication ID & Offset" stage.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT>
```

It'll then connect to your TCP server as a replica and execute the following commands:

1. `PING` (expecting `+PONG\r\n` back)
2. `REPLCONF listening-port <PORT>` (expecting `+OK\r\n` back)
3. `REPLCONF capa eof capa psync2` (expecting `+OK\r\n` back)
4. `PSYNC ? -1` (expecting `+FULLRESYNC <REPL_ID> 0\r\n` back)

**Notes**:

- In the response, `<REPL_ID>` needs to be replaced with the replication ID of the master which you've initialized in previous stages.

## Empty RDB Transfer

### Your Task

In this stage, you'll add support for sending an empty RDB file to the replica. This is part of the "full resynchronization" process.

### Full resynchronization

When a replica connects to a master for the first time, it sends a `PSYNC ? -1` command. This is the replica's way of telling the master that it doesn't have any data yet, and needs to be fully resynchronized.

The master acknowledges this by sending a `FULLRESYNC` response to the replica.

After sending the `FULLRESYNC` response, the master will  then send a RDB file of its current state to the replica. The replica is expected to load the file into memory, replacing its current state.

For the purposes of this challenge, you don't have to actually  construct an RDB file. We'll assume that the master's database is always empty, and just hardcode an empty RDB file to send to the replica.

You can find the hex representation of an empty RDB file [here](https://github.com/codecrafters-io/redis-tester/blob/main/internal/assets/empty_rdb_hex.md).

The tester will accept any valid RDB file that is empty, you don't need to send the exact file above.

The file is sent using the following format:

```
$<length_of_file>\r\n<contents_of_file>
```

(This is similar to how [Bulk Strings](https://redis.io/topics/protocol#resp-bulk-strings) are encoded, but without the trailing `\r\n`)

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT>
```

It'll then connect to your TCP server as a replica and execute the following commands:

1. `PING` (expecting `+PONG\r\n` back)
2. `REPLCONF listening-port <PORT>` (expecting `+OK\r\n` back)
3. `REPLCONF capa eof capa psync2` (expecting `+OK\r\n` back)
4. `PSYNC ? -1` (expecting `+FULLRESYNC <REPL_ID> 0\r\n` back)

After receiving a response to the last command, the tester will expect to receive an empty RDB file from your server.

### Notes

- The [RDB file link](https://github.com/codecrafters-io/redis-tester/blob/main/internal/assets/empty_rdb_hex.md) contains hex & base64 representations of the file. You need to decode these into binary contents before sending it to the replica.

- The RDB file should be sent like this: 

  ```
  $<length>\r\n<contents>
  ```

  - `<length>` is the length of the file in bytes
  - `<contents>` is the contents of the file
  - Note that this is NOT a RESP bulk string, it doesn't contain a `\r\n` at the end

- If you want to learn more about the RDB file format, read [this blog post](https://rdb.fnordig.de/file_format.html). This challenge has a separate extension dedicated to reading RDB files.

## Single-replica propagation

### Your Task

In this stage, you'll add support for propagating write commands from a master to a single replica.

### Command propagation

After the replication handshake is complete and the master has sent the RDB file to the replica, the master starts propagating commands to the replica.

When a master receives a "write" command from a client, it propagates the command to the replica. The replica processes the command and updates its state. More on how this propagation works in the "Replication connection" section below.

Commands like `PING`, `ECHO` etc. are not considered "write" commands, so they aren't propagated. Commands like `SET`, `DEL` etc. are considered "write" commands, so they are propagated.

### Replication connection

Command propagation happens over the replication connection. This is the same connection that was used for the handshake.

Propagated commands are sent as RESP arrays. For example, if the master receives `SET foo bar` as a command from a client, it'll send `*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n` to all connected replicas over their respective replication connections.

Replicas process commands received over the replication connection  just like they would process commands received from a client, but with one difference: Replicas don't send responses back to the  master. They just process the command silently and update their state.

Similarly, the master doesn't wait for a response from the replica  when propagating commands. It just keeps sending commands as they come in.

There is one exception to this "no response" rule, the `REPLCONF GETACK` command. We'll learn about this in later stages.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT>
```

It'll then connect to your TCP server as a replica and execute the following commands:

1. `PING` (expecting `+PONG\r\n` back)
2. `REPLCONF listening-port <PORT>` (expecting `+OK\r\n` back)
3. `REPLCONF capa eof capa psync2` (expecting `+OK\r\n` back)
4. `PSYNC ? -1` (expecting `+FULLRESYNC <REPL_ID> 0\r\n` back)

The tester will then wait for your server to send an RDB file.

Once the RDB file is received, the tester will send series of write  commands to your program (as a separate Redis client, not the replica).

```bash
$ redis-cli SET foo 1
$ redis-cli SET bar 2
$ redis-cli SET baz 3
```

It'll then assert that these commands were propagated to the replica, in order. The tester will expect to receive these commands (encoded as RESP arrays) on the replication connection (the one used for the handshake).

### Notes

- A true implementation would buffer the commands so that they can be  sent to the replica after it loads the RDB file. For the purposes of this challenge, you can assume that the replica is ready to  receive commands immediately after receiving the RDB file.

## Multi Replica Command Propagation

### Your Task

In this stage, you'll extend your implementation to support propagating commands to multiple replicas.

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT>
```

It'll then start **multiple** replicas that connect to your server and execute the following commands:

1. `PING` (expecting `+PONG\r\n` back)
2. `REPLCONF listening-port <PORT>` (expecting `+OK\r\n` back)
3. `REPLCONF capa psync2` (expecting `+OK\r\n` back)
4. `PSYNC ? -1` (expecting `+FULLRESYNC <REPL_ID> 0\r\n` back)

Each replica will expect to receive an RDB file from the master after the handshake is complete.

It'll then send `SET` commands to the master from a client (a separate Redis client, not the replicas).

```bash
$ redis-cli SET foo 1
$ redis-cli SET bar 2
$ redis-cli SET baz 3
```

It'll then assert that each replica received those commands, in order.

## Command Processing

### Your Task

In this stage you'll implement the processing of commands propagated to the replica from the master.

### Command processing

After the replica receives a command from the master, it processes it and apply it to its own state. This will work exactly like a regular command sent by a client, except that the replica doesn't send a response back to the master.

For example, if the command `SET foo 1` is propagated to the replica by a master, the replica must update its database to set the value of `foo` to `1`. Unlike commands from a regular client though, it must not reply with `+OK\r\n`.

### Tests

The tester will spawn a Redis master, and it'll then execute your program as a replica like this:

```
./your_program.sh --port <PORT> --replicaof "<MASTER_HOST> <MASTER_PORT>"
```

Just like in the previous stages, your replica should complete the handshake with the master and receive an empty RDB file.

Once the RDB file is received, the master will propagate a series of write commands to your program.

```bash
SET foo 1 # propagated from master to replica
SET bar 2 # propagated from master to replica
SET baz 3 # propagated from master to replica
```

The tester will then issue `GET` commands to your program to check if the commands were processed correctly.

```bash
$ redis-cli GET foo # expecting `1` back
$ redis-cli GET bar # expecting `2` back
# ... and so on
```

### Notes

- The propagated commands are sent as RESP arrays. So the command `SET foo 1` will be sent as `*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$1\r\n1\r\n`.
- It is **not** guaranteed that propagated commands will be sent one at a time. One "TCP segment" might contain bytes for multiple commands.

## ACKs with no commands

### Your Task

In this stage you'll implement support for the `REPLCONF GETACK` command that a master sends a replica.

### ACKs

Unlike regular commands, when a master forwards commands to a replica via the replication connection, the replica doesn't  respond to each command. It just silently processes the commands and updates its state.

Since the master doesn't receive a response for each command, it  needs another way to keep track of whether a replica is "in sync".  That's what ACKs are for.

ACK is short for "acknowledgement". Redis masters periodically ask replicas to send ACKs.

Each ACK contains an "offset", which is the number of bytes of commands processed by the replica.

We'll learn about how this offset is calculated and used in later stages. In this stage, we'll focus on implementing the  mechanism through which a master asks for an ACK from a replica: the `REPLCONF GETACK` command.

### The `REPLCONF GETACK` command

When a master requires an ACK from a replica, it sends a `REPLCONF GETACK *` command to the replica. This is sent over  the replication connection (i.e. the connection that remains after the replication handshake is complete).

When the replica receives this command, it responds with a `REPLCONF ACK <offset>` response. The offset is the  number of bytes of commands processed by the replica. It starts at 0  and is incremented for every command processed by the replica.

In this stage, you'll implement support for receiving the `REPLCONF GETACK *` command and responding with `REPLCONF ACK 0`.

You can hardcode the offset to 0 for now. We'll implement proper offset tracking in the next stage.

The exact command received by the replica will look something like this: `*3\r\n$8\r\nreplconf\r\n$6\r\ngetack\r\n$1\r\n*\r\n` (that's  `["replconf", "getack", "*"]` encoded as a [RESP Array](https://redis.io/docs/reference/protocol-spec/#arrays)).

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT> --replicaof "<HOST> <PORT>"
```

Just like in the previous stages, your replica should complete the handshake with the master and receive an empty RDB file.

The master will then send `REPLCONF GETACK *` to your replica. It'll expect to receive `REPLCONF ACK 0` as a reply.

### Notes

- The response should be encoded as a [RESP Array](https://redis.io/docs/reference/protocol-spec/#arrays), like this: `*3\r\n$8\r\nREPLCONF\r\n$3\r\nACK\r\n$1\r\n0\r\n`.
- We'll implement proper offset tracking in the next stage, for now you can hardcode the offset to 0.
- After the master-replica handshake is complete, a replica should **only** send responses to `REPLCONF GETACK` commands. All other propagated commands (like `PING`, `SET` etc.) should be read and processed, but a response should not be sent back to the master.

## ACKs with commands

### Your Task

In this stage, you'll extend your `REPLCONF GETACK` implementation to respond with the number of bytes of commands processed by the replica.

### Offset tracking

As we saw in previous stages, when a replica receives a command from the master, it processes it and updates its state. In addition to  processing  commands, the replica also keeps a running count of the number of  bytes of commands it has processed.

This count is called the "offset". When a master sends a `REPLCONF GETACK` command to a replica, the replica is expected to respond with  `REPLCONF ACK <offset>`. The returned `<offset>` should only include the number of bytes of commands processed **before** receiving the `REPLCONF GETACK` command.

As an example:

- Let's say a replica connects to a master and completes the handshake.
- The master then sends a `REPLCONF GETACK *` command.
  - The replica should respond with `REPLCONF ACK 0`.
  - The returned offset is 0 since no commands have been processed yet (before receiving the `REPLCONF GETACK` command)
- The master then sends `REPLCONF GETACK *` again.
  - The replica should respond with `REPLCONF ACK 37`.
  - The returned offset is 37 since the first `REPLCONF GETACK` command was processed, and it was 37 bytes long.
  - The RESP encoding for the `REPLCONF GETACK` command looks like this: ``*3\r\n$8\r\nreplconf\r\n$6\r\ngetack\r\n$1\r\n*\r\n` (that's 37 bytes long)
- The master then sends a `PING` command to the replica (masters do this periodically to notify replicas that the master is still alive).
  - The replica must silently process the `PING` command and update its offset. It should not send a response back to the master.
- The master then sends `REPLCONF GETACK *` again (this is the third REPLCONF GETACK command received by the replica)
  - The replica should respond with `REPLCONF ACK 88`.
  - The returned offset is 88 (37 + 37 + 14)
    - 37 for the first `REPLCONF GETACK` command
    - 37 for the second `REPLCONF GETACK` command
    - 14 for the `PING` command
  - Note that the third `REPLCONF GETACK` command is not included in the offset, since the value should only include the number of bytes of commands processed **before** receiving the current `REPLCONF GETACK` command.
- … and so on

### Tests

The tester will execute your program like this:

```
./your_program.sh --port <PORT> --replicaof "<HOST> <PORT>"
```

Just like in the previous stages, your replica should complete the handshake with the master and receive an empty RDB file.

The master will then propagate a series of commands to your replica. These commands will be interleaved with `REPLCONF GETACK *` commands.

```bash
REPLCONF getack * # expecting REPLCONF ACK 0, since 0 bytes have been processed

ping # master sending a ping command to notify the replica that it's still alive
REPLCONF getack * # expecting REPLCONF ACK 51 (37 for the first REPLCONF command + 14 for the ping command)

set foo 1 # propagated from master to replica
set bar 2 # propagated from master to replica
REPLCONF getack * # expecting REPLCONF ACK 109 (51 + 29 for the first set command + 29 for the second set command)
```

### Notes

- The offset should only include the number of bytes of commands processed **before** receiving the current `REPLCONF GETACK` command.
- Although masters don't propagate `PING` commands when received from clients (since they aren't "write" commands), they may send `PING` commands to replicas to notify replicas that the master is still alive.
- Replicas should update their offset to account for **all** commands propagated from the master, including `PING` and `REPLCONF` itself.
- The response should be encoded as a [RESP Array](https://redis.io/docs/reference/protocol-spec/#arrays), like this: `*3\r\n$8\r\nREPLCONF\r\n$3\r\nACK\r\n$3\r\n154\r\n`.

## WAIT with no replicas

### Your Task

**🚧 We're still working on instructions for this stage**. You can find notes on how the tester works below.

### Tests

The tester will execute your program like this:

```
./your_program.sh
```

A redis client will then connect to your master and send `WAIT 0 60000`:

```bash
$ redis-cli WAIT 0 60000
```

It'll expect to receive `0` back immediately, since no replicas are connected.

### Notes

- You can hardcode `0` as the response for the WAIT command in this stage. We'll get to tracking the number of replicas and responding accordingly in the next stages.

## WAIT with no commands

### Your Task

**🚧 We're still working on instructions for this stage**. You can find notes on how the tester works below.

### Tests

The tester will execute your program as a master like this:

```
./your_program.sh
```

It'll then start **multiple** replicas that connect to your server. Each will complete the handshake and expect to receive an empty RDB file.

It'll then connect to your master as a Redis client (not one of the replicas) and send commands like this:

```bash
$ redis-cli WAIT 3 500 # (expecting 7 back)
$ redis-cli WAIT 7 500 # (expecting 7 back)
$ redis-cli WAIT 9 500 # (expecting 7 back)
```

The response to each of these commands should be encoded as a RESP integer (i.e. `:7\r\n`).

### Notes

- Even if WAIT is called with a number lesser than the number of  connected replicas, the master should return the count of connected  replicas.
- The number of replicas created in this stage will be random, so you can't hardcode `7` as the response like in the example above.

## WAIT with multiple commands

### Your Task

**🚧 We're still working on instructions for this stage**. You can find notes on how the tester works below.

### Tests

The tester will execute your program as a master like this:

```
./your_program.sh
```

It'll then start **multiple** replicas that connect to your server. Each will complete the handshake and expect to receive an empty RDB file.

The tester will then connect to your master as a Redis client (not  one of the replicas) and send multiple write commands interleaved with `WAIT` commands:

```bash
$ redis-cli SET foo 123
$ redis-cli WAIT 1 500 # (must wait until either 1 replica has processed previous commands or 500ms have passed)

$ redis-cli SET bar 456
$ redis-cli WAIT 2 500 # (must wait until either 2 replicas have processed previous commands or 500ms have passed)
```

### Notes

- The `WAIT` command should return when either (a) the  specified number of replicas have acknowledged the command, or (b) the  timeout expires.
- The `WAIT` command should always return the number of replicas that have acknowledged the command, even if the timeout expires.
- The returned number of replicas might be lesser than or greater than the expected number of replicas specified in the `WAIT` command.

**Stream**

## The TYPE command

### Your Task

In this stage, you'll add support for the `TYPE` command.

### The TYPE command

The [TYPE](https://redis.io/commands/type/) command returns the type of value stored at a given key.

It returns one of the following types: string, list, set, zset, hash, and stream.

Here's how it works:

```bash
$ redis-cli SET some_key foo
"OK"
$ redis-cli TYPE some_key
"string"
```

If a key doesn't exist, the return value will be "none".

```bash
$ redis-cli TYPE missing_key
"none"
```

The return value is encoded as a [simple string](https://redis.io/docs/reference/protocol-spec/#simple-strings).

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then send a `SET` command to your server.

```bash
$ redis-cli SET some_key foo
```

It'll then send a `TYPE` command to your server.

```bash
$ redis-cli TYPE some_key
```

Your server should respond with `+string\r\n`, which is `string` encoded as a [RESP simple string](https://redis.io/docs/reference/protocol-spec/#simple-strings).

It'll then send another `TYPE` command with a missing key.

```bash
$ redis-cli TYPE missing_key
```

Your server should respond with `+none\r\n`, which is `none` encoded as a [RESP simple string](https://redis.io/docs/reference/protocol-spec/#simple-strings).

### Notes

- For now, you only need to handle the "string" and "none" types. We'll add support for the "stream" type in the next stage.

## Create a stream

### Your Task

In this stage, you'll add support for creating a [Redis stream](https://redis.io/docs/data-types/streams/) using the `XADD` command.

### Redis Streams & Entries

Streams are one of the data types that Redis supports. A stream is identified by a key, and it contains multiple entries.

Each entry consists of one or more key-value pairs, and is assigned a unique ID.

For example, if you were using a Redis stream to store real-time data from a temperature & humidity monitor, the contents of the stream  might look like this:

```yaml
entries:
  - id: 1526985054069-0 # (ID of the first entry)
    temperature: 36 # (A key value pair in the first entry)
    humidity: 95 # (Another key value pair in the first entry)

  - id: 1526985054079-0 # (ID of the second entry)
    temperature: 37 # (A key value pair in the first entry)
    humidity: 94 # (Another key value pair in the first entry)

  # ... (and so on)
```

We'll take a closer look at the format of entry IDs (`1526985054069-0` and `1526985054079-0` in the example above) in the upcoming stages.

### The XADD command

The [XADD](https://redis.io/commands/xadd/) command appends an entry to a stream. If a stream doesn't exist already, it is created.

Here's how it works:

```bash
$ redis-cli XADD stream_key 1526919030474-0 temperature 36 humidity 95
"1526919030474-0" # (ID of the entry created)
```

The return value is the ID of the entry created, encoded as a [bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings).

`XADD` supports other optional arguments, but we won't deal with them in this challenge.

`XADD` also supports auto-generating entry IDs. We'll add support for that in later stages. For now, we'll only deal with explicit IDs (like `1526919030474-0` in the example above).

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then send an `XADD` command to your server and expect the ID as a response.

```bash
$ redis-cli XADD stream_key 0-1 foo bar
"0-1"
```

Your server should respond with `$3\r\n0-1\r\n`, which is `0-1` encoded as a [RESP bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings).

The tester will then send a `TYPE` command to your server.

```bash
$ redis-cli TYPE stream_key
"stream"
```

Your server should respond with `+stream\r\n`, which is `stream` encoded as a [RESP simple string](https://redis.io/docs/reference/protocol-spec/#simple-strings).

### Notes

- You still need to handle the "string" and "none" return values for the `TYPE` command. "stream" should only be returned for keys that are streams.

## Validating entry IDs

### Your Task

In this stage, you'll add support for validating entry IDs to the `XADD` command.

### Entry IDs

Here's an example of stream entries from the previous stage:

```yaml
entries:
  - id: 1526985054069-0 # (ID of the first entry)
    temperature: 36 # (A key value pair in the first entry)
    humidity: 95 # (Another key value pair in the first entry)

  - id: 1526985054079-0 # (ID of the second entry)
    temperature: 37 # (A key value pair in the first entry)
    humidity: 94 # (Another key value pair in the first entry)

  # ... (and so on)
```

Entry IDs are always composed of two integers: `<millisecondsTime>-<sequenceNumber>`.

Entry IDs are unique within a stream, and they're guaranteed to be incremental - i.e. an entry added later will always have an ID greater than an entry added in the past. More on this in the next section.

### Specifying entry IDs in XADD

There are multiple formats in which the ID can be specified in the XADD command:

- Explicit ("1526919030474-0") (**This stage**)
- Auto-generate only sequence number ("1526919030474-*") (Next stages)
- Auto-generate time part and sequence number ("*") (Next stages)

In this stage, we'll only deal with explicit IDs. We'll add support for the other two cases in the next stages.

Your XADD implementation should validate the ID passed in.

- The ID should be greater than the ID of the last entry in the stream.
  - The `millisecondsTime` part of the ID should be greater than or equal to the `millisecondsTime` of the last entry.
  - If the `millisecondsTime` part of the ID is equal to the `millisecondsTime` of the last entry, the `sequenceNumber` part of the ID should be greater than the `sequenceNumber` of the last entry.
- If the stream is empty, the ID should be greater than `0-0`

Here's an example of adding an entry with a valid ID followed by an invalid ID:

```bash
$ redis-cli XADD some_key 1-1 foo bar
"1-1"
$ redis-cli XADD some_key 1-1 bar baz
(error) ERR The ID specified in XADD is equal or smaller than the target stream top item
```

Here's another such example:

```bash
$ redis-cli XADD some_key 1-1 foo bar
"1-1"
$ redis-cli XADD some_key 0-2 bar baz
(error) ERR The ID specified in XADD is equal or smaller than the target stream top item
```

The minimum entry ID that Redis supports is 0-1. Passing in an ID lower than should result in an error.

```bash
$ redis-cli XADD some_key 0-0 bar baz
(error) ERR The ID specified in XADD must be greater than 0-0
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll create a few entries usind `XADD`.

```bash
$ redis-cli XADD stream_key 1-1 foo bar
"1-1"
$ redis-cli XADD stream_key 1-2 bar baz
"1-2"
```

It'll send another `XADD` command with the same time and sequence number as the last entry.

```bash
$ redis-cli XADD stream_key 1-2 baz foo
(error) ERR The ID specified in XADD is equal or smaller than the target stream top item
```

Your server should respond with "-ERR The ID specified in XADD is  equal or smaller than the target stream top item\r\n", which is the  error message above encoded as a [simple error](https://redis.io/docs/reference/protocol-spec/#simple-errors).

The tester will then send another `XADD` command with a smaller value for the time and a larger value for the sequence number.

```bash
$ redis-cli XADD stream_key 0-3 baz foo
(error) ERR The ID specified in XADD is equal or smaller than the target stream top item
```

Your server should also respond with the same error message.

After that, the tester will send another `XADD` command with `0-0` as the ID.

```bash
$ redis-cli XADD stream_key 0-0 baz foo
```

Your server should respond with "-ERR The ID specified in XADD must  be greater than 0-0\r\n", which is the error message above encoded as a [RESP simple error](https://redis.io/docs/reference/protocol-spec/#simple-errors).

## Partially auto-generated IDs

### Your Task

In this stage, you'll extend your `XADD` command implementation to support auto-generating the sequence number part of the entry ID.

### Specifying entry IDs in XADD

As a recap, there are multiple formats in which the ID can be specified in the `XADD` command:

- Explicit ("1526919030473-0") (Previous stage)
- Auto-generate only sequence number ("1526919030474-*") (**This stage**)
- Auto-generate time part and sequence number ("*") (Next stage)

We dealt with explicit IDs in the last stage. We'll handle the second case in this stage.

When `*` is used for the sequence number, Redis picks the last sequence number used in the stream (for the same time part) and increments it by 1.

The default sequence number is 0. The only exception is when the time part is also 0. In that case, the default sequence number is 1.

Here's an example of adding an entry with `*` as the sequence number:

```bash
$ redis-cli XADD some_key "1-*" foo bar
"1-0" # If there are no entries, the sequence number will be 0
$ redis-cli XADD some_key "1-*" bar baz
"1-1" # Adding another entry will increment the sequence number
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll send an `XADD` command with `*` as the sequence number.

```bash
$ redis-cli XADD stream_key 0-* foo bar
```

Your server should respond with `$3\r\n0-1\r\n`, which is `0-1` encoded as a RESP bulk string.

It'll then send another `XADD` command with `*` as the sequence number, but this time with a random number as the time part.

```bash
$ redis-cli XADD stream_key 5-* foo bar
```

Your server should respond with `$3\r\n5-0\r\n`, which is `5-0` encoded as a [RESP bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings)

It'll send the same command again.

```bash
$ redis-cli XADD stream_key 5-* bar baz
```

Your server should respond with `$3\r\n5-1\r\n`, which is `5-1` encoded as a [RESP bulk string](https://redis.io/docs/reference/protocol-spec/#bulk-strings)

### Notes

- The tester will use a random number for the time part (we use `5` in the example above).

## Fully auto-generated IDs

### Your Task

In this stage, you'll extend your `XADD` command implementation to support auto-generating entry IDs.

### Specifying entry IDs in XADD (Continued…)

As a recap, there are multiple formats in which the ID can be specified in the `XADD` command:

- Explicit ("1526919030474-0") (Previous stages)
- Auto-generate only sequence number ("1526919030473-*") (Previous stages)
- Auto-generate time part and sequence number ("*") (**This stage**)

We'll now handle the third case.

When `*` is used with the `XADD` command, Redis auto-generates a unique auto-incrementing ID for the message being appended to the stream.

Redis defaults to using the current unix time in milliseconds for the time part and 0 for the sequence number. If the time already exists in the stream, the sequence number for that record incremented by one will be used.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then create an entry with `*` as the ID.

```bash
$ redis-cli XADD stream_key * foo bar
```

Your server should respond with a string like `$15\r\n1526919030474-0\r\n`, which is `1526919030474-0` encoded as a RESP bulk string.

### Notes

- The time part of the ID should be the current unix time in **milliseconds**, not seconds.
- The tester doesn't test the case where a time part already exists in the stream and the sequence number is incremented. This is difficult to test reliably since we'd need to send 2 commands within the same millisecond.

## Query entries from stream

### Your Task

In this stage, you'll add support for querying data from a stream using the `XRANGE` command.

### The XRANGE command

The [XRANGE](https://redis.io/commands/xrange/) command retrieves a range of entries from a stream.

It takes two arguments: `start` and `end`. Both are entry IDs. The command returns all entries in the stream with IDs between the `start` and `end` IDs. This range is "inclusive", which means that the response will includes entries with IDs that are equal to the `start` and `end` IDs.

Here's an example of how it works:

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0" # (ID of the first added entry)
$ redis-cli XADD some_key 1526985054079-0 temperature 37 humidity 94
"1526985054079-0"
$ redis-cli XRANGE some_key 1526985054069 1526985054079
1) 1) 1526985054069-0
   2) 1) temperature
      2) 36
      3) humidity
      4) 95
2) 1) 1526985054079-0
   2) 1) temperature
      2) 37
      3) humidity
      4) 94
```

The sequence number doesn't need to be included in the start and end IDs provided to the command. If not provided, XRANGE defaults to a sequence number of 0 for the start and the maximum sequence number for the end.

The return value of the command is not exactly what is shown in the example above. This is already formatted by redis-cli.

The actual return value is a [RESP Array](https://redis.io/docs/reference/protocol-spec/#arrays) of arrays.

- Each inner array represents an entry.
- The first item in the inner array is the ID of the entry.
- The second item is a list of key value pairs, where the key value pairs are represented as a list of strings.
  - The key value pairs are in the order they were added to the entry.

The return value of the example above is actually something like this:

```json
[
  [
    "1526985054069-0",
    [
      "temperature",
      "36",
      "humidity",
      "95"
    ]
  ],
  [
    "1526985054079-0",
    [
      "temperature",
      "37",
      "humidity",
      "94"
    ]
  ],
]
```

When encoded as a RESP list, it looks like this:

```text
*2\r\n
*2\r\n
$15\r\n1526985054069-0\r\n
*4\r\n
$11\r\ntemperature\r\n
$2\r\n36\r\n
$8\r\nhumidity\r\n
$2\r\n95\r\n
*2\r\n
$15\r\n1526985054079-0\r\n
*4\r\n
$11\r\ntemperature\r\n
$2\r\n37\r\n
$8\r\nhumidity\r\n
$2\r\n94\r\n
```

In the code block above, the response is separated into multiple lines for readability. The actual return value doesn't contain any additional newlines.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

First, it'll add a few entries.

```bash
$ redis-cli XADD stream_key 0-1 foo bar
"0-1"
$ redis-cli XADD stream_key 0-2 bar baz
"0-2"
$ redis-cli XADD stream_key 0-3 baz foo
"0-3"
```

Then, it'll send an `XRANGE` command to your server.

```bash
$ redis-cli XRANGE stream_key 0-2 0-3
```

Your server should respond with the following (encoded as a RESP Array):

```json
[
  [
    "0-2",
    [
      "bar",
      "baz"
    ]
  ],
  [
    "0-3",
    [
      "baz",
      "foo"
    ]
  ]
]
```

## Query with -

### Your Task

In this stage, you'll extend support for `XRANGE` to allow querying using `-`.

### Using XRANGE with -

In the previous stage, we saw that `XRANGE` takes `start` and `end` as arguments.

In addition to accepting an explicit entry ID, `start` can also be specified as `-`. When `-` is used, `XRANGE` retrieves entries from the beginning of the stream.

Here's an example of how that works.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XADD some_key 1526985054079-0 temperature 37 humidity 94
"1526985054079-0"
$ redis-cli XRANGE some_key - 1526985054079
1) 1) 1526985054069-0
   2) 1) temperature
      2) 36
      3) humidity
      4) 95
2) 1) 1526985054079-0
   2) 1) temperature
      2) 37
      3) humidity
      4) 94
```

In the example above, `XRANGE` retrieves all entries from the beginning of the stream to the entry with ID `1526985054079-0`.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then create a few entries.

```bash
$ redis-cli XADD stream_key 0-1 foo bar
"0-1"
$ redis-cli XADD stream_key 0-2 bar baz
"0-2"
$ redis-cli XADD stream_key 0-3 baz foo
"0-3"
```

It'll then send an `XRANGE` command to your server.

```bash
$ redis-cli XRANGE stream_key - 0-2
1) 1) 0-1
   2) 1) foo
      2) bar
2) 1) 0-2
   2) 1) bar
      2) baz
```

Your server should respond with the following, encoded as a [RESP Array](https://redis.io/docs/reference/protocol-spec/#arrays):

```json
[
  [
    "0-1",
    [
      "foo",
      "bar"
    ]
  ],
  [
    "0-2",
    [
      "bar",
      "baz"
    ]
  ]
]
```

## Query with +

### Your Task

In this stage, you'll extend support for `XRANGE` to allow querying using `+`.

### Using XRANGE with +

In the previous stage, we saw that `XRANGE` takes `start` and `end` as arguments.

In addition to accepting an explicit entry ID, `end` can also be specified as `+`. When `+` is used, `XRANGE` retrieves entries until the end of the stream.

Here's an example of how that works.

We'll use our previous example for entries existing in a stream.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XADD some_key 1526985054079-0 temperature 37 humidity 94
"1526985054079-0"
$ redis-cli XRANGE some_key 1526985054069 +
1) 1) 1526985054069-0
   2) 1) temperature
      2) 36
      3) humidity
      4) 95
2) 1) 1526985054079-0
   2) 1) temperature
      2) 37
      3) humidity
      4) 94
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

It'll then create a few entries.

```bash
$ redis-cli XADD stream_key 0-1 foo bar
$ redis-cli XADD stream_key 0-2 bar baz
$ redis-cli XADD stream_key 0-3 baz foo
```

It'll then send an `XRANGE` command to your server.

```bash
$ redis-cli XRANGE stream_key 0-2 +
```

Your server should respond with the following:

```text
*2\r\n
*2\r\n
$3\r\n0-2\r\n
*2\r\n
$3\r\nbar\r\n
$3\r\nbaz\r\n
*2\r\n
$3\r\n0-3\r\n
*2\r\n
$3\r\nbaz\r\n
$3\r\nfoo\r\n
```

This is the RESP encoded representation of the following.

```json
[
  [
    "0-2",
    [
      "bar",
      "baz"
    ]
  ],
  [
    "0-3",
    [
      "baz",
      "foo"
    ]
  ]
]
```

### Notes

- In the response, the items are separated onto new lines for readability. The tester expects all of these to be in one line.

## Query single stream using XREAD

### Your Task

In this stage, you'll add support to querying a stream using the `XREAD` command.

### The XREAD command

[XREAD](https://redis.io/commands/xread/) is used to read data from one or more streams, starting from a specified entry ID.

Here's how it works.

Let's use the entries previously shown as an example.

```yaml
entries:
  - id: 1526985054069-0 # (ID of the first entry)
    temperature: 36 # (A key value pair in the first entry)
    humidity: 95 # (Another key value pair in the first entry)

  - id: 1526985054079-0 # (ID of the second entry)
    temperature: 37 # (A key value pair in the first entry)
    humidity: 94 # (Another key value pair in the first entry)

  # ... (and so on)
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XADD some_key 1526985054079-0 temperature 37 humidity 94
"1526985054079-0"
$ redis-cli XREAD streams some_key 1526985054069-0
1) 1) "some_key"
   2) 1) 1) 1526985054079-0
         2) 1) temperature
            2) 37
            3) humidity
            4) 94
```

`XREAD` is somewhat similar to `XRANGE`. The primary difference is that `XREAD` only takes a single argument and not a start-end pair.

Another difference is that `XREAD` is exclusive. This means that only entries with the ID greater than what was provided will be included in the response.

Another difference is the return type. `XREAD` returns an array where each element is an array composed of two elements, which are the ID and the list of fields and values.

Here's what the response in the example above actually looks like:

```json
[
  [
    "some_key",
    [
      [
        "1526985054079-0",
        [
          "temperature",
          "37",
          "humidity",
          "94"
        ]
      ]
    ]
  ]
]
```

When encoded as RESP, it looks like this:

```text
*1\r\n
*2\r\n
$8\r\nsome_key\r\n
*1\r\n
*2\r\n
$15\r\n1526985054079-0\r\n
*4\r\n
$11\r\ntemperature\r\n
$2\r\n37\r\n
$8\r\nhumidity\r\n
$2\r\n94\r\n
```

The lines are separated into new lines for readability. The return value is just one line.

`XREAD` supports other optional arguments, but we won't deal with them right now.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

First, an entry will be added.

```bash
$ redis-cli XADD stream_key 0-1 temperature 96
```

It'll then send an `XREAD` command to your server.

```bash
$ redis-cli XREAD streams stream_key 0-0
```

Your server should respond with the following:

```text
*1\r\n
*2\r\n
$10\r\nstream_key\r\n
*1\r\n
*2\r\n
$3\r\n0-1\r\n
*2\r\n
$11\r\ntemperature\r\n
$2\r\n96\r\n
```

This is the RESP encoded representation of the following.

```json
[
  [
    "stream_key",
    [
      [
        "0-1",
        [
          "temperature",
          "96"
        ]
      ]
    ]
  ]
]
```

### Notes

- In the response, the items are separated onto new lines for readability. The tester expects all of these to be in one line.



## Query multiple streams using XREAD



### Your Task

In this stage, you'll add extend support to `XREAD` to allow querying multiple streams.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

First, an entry will be added to a couple of streams.

```bash
$ redis-cli XADD stream_key 0-1 temperature 95
$ redis-cli XADD other_stream_key 0-2 humidity 97
```

It'll then send an `XREAD` command to your server with multiple streams.

```bash
$ redis-cli XREAD streams stream_key other_stream_key 0-0 0-1
```

Your server should respond with the following:

```text
*2\r\n
*2\r\n
$10\r\nstream_key\r\n
*1\r\n
*2\r\n
$3\r\n0-1\r\n
*2\r\n
$11\r\ntemperature\r\n
$2\r\n95\r\n
*2\r\n
$16\r\nother_stream_key\r\n
*1\r\n
*2\r\n
$3\r\n0-2\r\n
*2\r\n
$8\r\nhumidity\r\n
$2\r\n97\r\n
```

This is the RESP encoded representation of the following.

```json
[
  [
    "stream_key",
    [
      [
        "0-1",
        [
          "temperature",
          "95"
        ]
      ]
    ]
  ],
  [
    "other_stream_key",
    [
      [
        "0-2",
        [
          "humidity",
          "97"
        ]
      ]
    ]
  ]
]
```

### Notes

- In the response, the items are separated onto new lines for readability. The tester expects all of these to be in one line.

## Blocking reads

### Your Task

In this stage, you'll extend support to `XREAD` to allow for turning it into a blocking command.

### Understanding blocking

`BLOCK` is one of the optional parameters that could be passed in to the `XREAD` command.

Without blocking, the current implementation of our command is  synchronous. This means that the command can get new data as long as  there are items available.

If we want to wait for new data coming in, we need blocking.

Here's how it works.

Let's use the entries previously shown as an example.

```yaml
entries:
  - id: 1526985054069-0 # (ID of the first entry)
    temperature: 36 # (A key value pair in the first entry)
    humidity: 95 # (Another key value pair in the first entry)

  - id: 1526985054079-0 # (ID of the second entry)
    temperature: 37 # (A key value pair in the first entry)
    humidity: 94 # (Another key value pair in the first entry)

  # ... (and so on)
```

On one instance of the redis-cli, we'd add an entry and send  a blocking `XREAD` command.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 1000 streams some_key 1526985054069-0
```

Then, on another instance of the redis-cli, we add another entry.

```bash
$ other-redis-cli XADD some_key 1526985054079-0 temperature 37 humidity 94
"1526985054079-0"
```

If the command was sent within 1000 milliseconds, the redis-cli will respond with the added entry.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 1000 streams some_key 1526985054069-0
1) 1) "some_key"
   2) 1) 1) 1526985054079-0
         2) 1) temperature
            2) 37
            3) humidity
            4) 94
```

If not, the response would be a null representation of a bulk string.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 1000 streams some_key 1526985054069-0
(nil)
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

First, an entry will be added to a stream.

```bash
$ redis-cli XADD stream_key 0-1 temperature 96
```

It'll then send an `XREAD` command to your server with the `BLOCK` command.

```bash
$ redis-cli XREAD block 1000 streams stream_key 0-1
```

On another instance of the redis-cli, another entry will be added in 500 milliseconds after sending the `XREAD` command.

```bash
$ redis-cli XADD stream_key 0-2 temperature 95
```

Your server should respond with the following:

```text
*1\r\n
*2\r\n
$10\r\stream_key\r\n
*1\r\n
*2\r\n
$3\r\n0-2\r\n
*2\r\n
$11\r\ntemperature\r\n
$2\r\n96\r\n
```

This is the RESP encoded representation of the following.

```json
[
  [
    "stream_key",
    [
      "0-2",
      [
        "temperature",
        "96"
      ]
    ]
  ]
]
```

It'll send another `XREAD` command to your server with the `BLOCK` command but this time, it'll wait for 2000 milliseconds before checking the response of your server.

```bash
$ redis-cli XREAD block 1000 streams stream_key 0-2
```

Your server should respond with `$-1\r\n` which is a `null` representation of a RESP bulk string.

### Notes

- In the response, the items are separated onto new lines for readability. The tester expects all of these to be in one line.



## Blocking reads without timeout

### Your Task

In this stage, you'll extend support to `XREAD` to allow for the blocking command not timing out.

### Understanding blocking without timeout

Here's how it works.

Let's use the entries previously shown as an example.

```yaml
entries:
  - id: 1526985054069-0 # (ID of the first entry)
    temperature: 36 # (A key value pair in the first entry)
    humidity: 95 # (Another key value pair in the first entry)

  - id: 1526985054079-0 # (ID of the second entry)
    temperature: 37 # (A key value pair in the first entry)
    humidity: 94 # (Another key value pair in the first entry)

  # ... (and so on)
```

On one instance of the redis-cli, we'd add an entry and send a blocking `XREAD` command with 0 as the time passed in.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 0 streams some_key 1526985054069-0
```

Then, on another instance of the redis-cli, we add another entry.

```bash
$ other-redis-cli XADD some_key 1526985054079-0 temperature 37 humidity 94
"1526985054079-0"
```

The difference now is that the first instance of the redis-cli  doesn't time out and responds with null no matter how much time passes.  It will wait until another entry is added. The return value after an  entry is added is similar to the last stage.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 0 streams some_key 1526985054069-0
1) 1) "some_key"
   2) 1) 1) 1526985054079-0
         2) 1) temperature
            2) 37
            3) humidity
            4) 94
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

First, an entry will be added to a stream.

```bash
$ redis-cli XADD stream_key 0-1 temperature 96
```

It'll then send an `XREAD` command to your server with the `BLOCK` command with the time passed in being 0.

```bash
$ redis-cli XREAD block 0 streams stream_key 0-0
```

It'll then wait for 1000 milliseconds before checking if there is a  response. Your server should not have a new response. It'll then add  another entry.

```bash
$ redis-cli XADD stream_key 0-2 temperature 95
```

Your server should respond with the following:

```text
*1\r\n
*2\r\n
$10\r\stream_key\r\n
*1\r\n
*2\r\n
$3\r\n0-2\r\n
*2\r\n
$11\r\ntemperature\r\n
$2\r\n95\r\n
```

This is the RESP encoded representation of the following.

```json
[
  [
    "stream_key",
    [
      "0-2",
      [
        "temperature",
        "95"
      ]
    ]
  ]
]
```

### Notes

- In the response, the items are separated onto new lines for readability. The tester expects all of these to be in one line.

## Blocking reads using `$`

### Your Task

In this stage, you'll extend support to `XREAD` to allow for passing in `$` as the ID for a blocking command.

### Understanding $

Using `$` as the ID passed to a blocking `XREAD` command signals that we only want new entries. This is similar to passing in the maximum ID we currently have in the stream.

Here's how it works.

Let's use the entries previously shown as an example.

```yaml
entries:
  - id: 1526985054069-0 # (ID of the first entry)
    temperature: 36 # (A key value pair in the first entry)
    humidity: 95 # (Another key value pair in the first entry)

  - id: 1526985054079-0 # (ID of the second entry)
    temperature: 37 # (A key value pair in the first entry)
    humidity: 94 # (Another key value pair in the first entry)

  # ... (and so on)
```

On one instance of the redis-cli, we'd add an entry and send a blocking `XREAD` command with `1000` as the time passed in and `$` as the id passed in.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 1000 streams some_key $
```

Then, on another instance of the redis-cli, we add another entry.

```bash
$ other-redis-cli XADD some_key 1526985054079-0 temperature 37 humidity 94
"1526985054079-0"
```

Similar to the behavior detailed in the earlier stages, if the  command was sent within 1000 milliseconds, the redis-cli will respond  with the new entry.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 1000 streams some_key 1526985054069-0
1) 1) "some_key"
   2) 1) 1) 1526985054079-0
         2) 1) temperature
            2) 37
            3) humidity
            4) 94
```

If not, the return type would still be a null representation of a bulk string.

```bash
$ redis-cli XADD some_key 1526985054069-0 temperature 36 humidity 95
"1526985054069-0"
$ redis-cli XREAD block 1000 streams some_key 1526985054069-0
(nil)
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

First, an entry will be added to a stream.

```bash
$ redis-cli XADD stream_key 0-1 temperature 96
```

It'll then send an `XREAD` command to your server with the `BLOCK` command with `0` as the time and `$` as the ID.

```bash
$ redis-cli XREAD block 0 streams stream_key $
```

On another instance of the redis-cli, another entry will be added in 500 milliseconds after sending the `XREAD` command.

```bash
$ redis-cli XADD stream_key 0-2 temperature 95
```

Your server should respond with the following:

```text
*1\r\n
*2\r\n
$10\r\stream_key\r\n
*1\r\n
*2\r\n
$3\r\n0-2\r\n
*2\r\n
$11\r\ntemperature\r\n
$2\r\n96\r\n
```

This is the RESP encoded representation of the following.

```json
[
  [
    "stream_key",
    [
      "0-2",
      [
        "temperature",
        "95"
      ]
    ]
  ]
]
```

It'll send another `XREAD` command to your server with the `BLOCK` command but this time, it'll wait for 2000 milliseconds before checking the response of your server.

```bash
$ redis-cli XREAD block 1000 streams stream_key $
```

Your server should respond with `$-1\r\n` which is a `null` representation of a RESP bulk string.

### Notes

- In the response, the items are separated onto new lines for readability. The tester expects all of these to be in one line.



**Transactions**

## The INCR command (1/3)

### Your Task

In this stage, you'll add support for the `INCR` command.

### The INCR command

The [INCR](https://redis.io/docs/latest/commands/incr/) command is used to increment the value of a key by 1.

Example usage:

```bash
$ redis-cli SET foo 5
"OK"
$ redis-cli INCR foo
(integer) 6
$ redis-cli INCR foo
(integer) 7
```

If the key doesn't exist, the value will be set to 1.

We'll split the implementation of this command into three stages:

- Key exists and has a numerical value (**This stage**)
- Key doesn't exist (later stages)
- Key exists but doesn't have a numerical value (later stages)

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client and run the following commands:

```bash
$ redis-cli
> SET foo 41 (expecting "+OK" as the response)
> INCR foo (expecting ":42\r\n" as the response)
```

## The INCR command (2/3)

### Your Task

 In this stage, you'll add support for handling the `INCR` command when a key does not exist.

### Recap

The implementation of [`INCR`](https://redis.io/docs/latest/commands/incr/) is split into three stages:

- Key exists and has a numerical value (previous stages)
- Key doesn't exist (**This stage**)
- Key exists but doesn't have a numerical value (later stages)

When a key doesn't exist, `INCR` sets the value to 1. Example:

```bash
$ redis-cli INCR missing_key
(integer) 1
$ redis-cli GET missing_key
"1"
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client and run the following commands:

```bash
$ redis-cli
> INCR foo (expecting ":1\r\n" as the response)
> INCR bar (expecting ":1\r\n" as the response)
```

### Notes

- Your implementation still needs to pass the tests in the previous stage.

## The INCR command (3/3)

### Your Task

In this stage, you'll add support for handling the `INCR` command when a key exists but doesn't have a numerical value.

### Recap

The implementation of [`INCR`](https://redis.io/docs/latest/commands/incr/) is split into three stages:

- Key exists and has a numerical value (previous stages)
- Key doesn't exist (previous stages)
- Key exists but doesn't have a numerical value (**This stage**)

When a key exists but doesn't have a numerical value, `INCR` will return an error. Example:

```bash
$ redis-cli SET foo xyz
"OK"
$ redis-cli INCR foo
(error) ERR value is not an integer or out of range
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client and run the following commands:

```bash
$ redis-cli
> SET foo xyz (expecting "+OK\r\n" as the response)
> INCR foo (expecting "-ERR value is not an integer or out of range\r\n" as the response)
```

 

## The MULTI command

### Your Task

In this stage, you'll add support for the `MULTI` command.

### The MULTI command

The [MULTI](https://redis.io/docs/latest/commands/multi/) command starts a transaction.

After a `MULTI` command is executed, any further commands from the same connection will be "queued" but not executed.

Example usage:

```bash
$ redis-cli
> MULTI
OK
> SET foo 41
QUEUED
> INCR foo
QUEUED
```

The queued commands can be executed using [EXEC](https://redis.io/docs/latest/commands/exec/), which we'll cover in later stages.

In this stage, you'll just add support for handling the `MULTI` command and returning `+OK\r\n`. We'll get to queueing commands in later stages.

### Tests

The tester will execute your program like this:

```
./your_program.sh
```

The tester will then connect to your server as a Redis client and run the following command:

```bash
$ redis-cli MULTI
```

The tester will expect `+OK\r\n` as the response.

### Notes

- We'll test queueing commands & executing a transaction in later stages.

## The EXEC command

### Your Task

In this stage, you'll add support for the `EXEC` command when the `MULTI` command has not been called.

### The EXEC command

The [EXEC](https://redis.io/docs/latest/commands/exec/) command executes all commands queued in a transaction.

It returns an array of the responses of the queued commands.

Example usage:

```bash
$ redis-cli
> MULTI
OK
> SET foo 41
QUEUED
> INCR foo
QUEUED
> EXEC
1) OK
2) (integer) 42
```

### EXEC without MULTI

If `EXEC` is executed without having called `MULTI`, it returns an error.

Example usage:

```bash
$ redis-cli EXEC
(error) ERR EXEC without MULTI
```

The returned value is a [Simple error](https://redis.io/docs/latest/develop/reference/protocol-spec/#simple-errors), the exact bytes are `-ERR EXEC without MULTI\r\n`.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client and run the following commands:

```bash
$ redis-cli EXEC
```

The tester will expect "-ERR EXEC without MULTI\r\n" as the response.

### Notes

- In this stage you only need to implement `EXEC` when `MULTI` hasn't been called.
- We'll test handling `EXEC` after `MULTI` in later stages.

## Empty transaction

### Your Task

In this stage, you'll add support for executing an empty transaction.

### Empty trasactions

If [EXEC](https://redis.io/docs/latest/commands/exec/) is executed soon after [MULTI](https://redis.io/docs/latest/commands/multi/), it returns an empty array.

The empty array signifies that no commands were queued, and that the transaction was executed successfully.

Example usage:

```bash
$ redis-cli
> MULTI
OK
> EXEC
(empty array)
```

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client and run the following commands:

```bash
$ redis-cli
> MULTI (expecting "+OK\r\n")
> EXEC (expecting "*0\r\n" as the response)
> EXEC (expecting "-ERR EXEC without MULTI\r\n" as the response)
```



## Queueing commands

### Your Task

In this stage, you'll add support for queuing commands within a transaction.

### Queuing commands

After [MULTI](https://redis.io/docs/latest/commands/multi/) is executed, any further commands from a connection are queued until [EXEC](https://redis.io/docs/latest/commands/exec/) is executed.

The response to all of these commands is `+QUEUED\r\n` (That's `QUEUED` encoded as a [Simple String](https://redis.io/docs/latest/develop/reference/protocol-spec/#simple-strings)).

Example:

```bash
$ redis-cli
> MULTI
OK
> SET foo 41
QUEUED
> INCR foo
QUEUED

... (and so on, until EXEC is executed)
```

When commands are queued, they should not be executed or alter the database in any way.

In the example above, until `EXEC` is executed, the key `foo` will not exist.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client, and send multiple commands using the same connection:

```bash
$ redis-cli
> MULTI
> SET foo 41 (expecting "+QUEUED\r\n")
> INCR foo (expecting "+QUEUED\r\n")
```

Since these commands were only "queued", the key `foo` should not exist yet. The tester will verify this by creating another connection and sending this command:

```bash
$ redis-cli GET foo (expecting `$-1\r\n` as the response)
```

## Executing a transaction

### Your Task

In this stage, you'll add support for executing a transaction that contains multiple commands.

### Executing a transaction

When the [EXEC](https://redis.io/docs/latest/commands/exec/) command is sent within a transaction, all commands queued in that transaction are executed.

The response to [EXEC](https://redis.io/docs/latest/commands/exec/) is an array of the responses of the queued commands.

Example:

```bash
$ redis-cli
> MULTI
OK
> SET foo 41
QUEUED
> INCR foo
QUEUED
> EXEC
1) OK
2) (integer) 42
```

In the above example, `OK` is the response of the `SET` command, and `42` is the response of the `INCR` command.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client, and send multiple commands using the same connection:

```bash
$ redis-cli MULTI
> SET foo 6 (expecting "+QUEUED\r\n")
> INCR foo (expecting "+QUEUED\r\n")
> INCR bar (expecting "+QUEUED\r\n")
> GET bar (expecting "+QUEUED\r\n")
> EXEC (expecting an array of responses for the queued commands)
```

Since the transaction was executed, the key `foo` should exist and have the value `7`.

The tester will verify this by running a GET command:

```bash
$ redis-cli GET foo (expecting "7" encoded as a bulk string)
```

## The DISCARD command

### Your Task

In this stage, you'll add support for the DISCARD command.

### The DISCARD command

[DISCARD](https://redis.io/docs/latest/commands/discard/) is used to abort a transactions. It discards all commands queued in a transaction, and returns `+OK\r\n`.

Example:

```bash
$ redis-cli
> MULTI
OK
> SET foo 41
QUEUED
> DISCARD
OK
> DISCARD
(error) ERR DISCARD without MULTI
```

In the above example, note that the first `DISCARD` returns `OK`, but the second `DISCARD` returns an error since the transaction was aborted.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client, and send multiple commands using the same connection:

```bash
$ redis-cli
> MULTI
> SET foo 41 (expecting "+QUEUED\r\n")
> INCR foo (expecting "+QUEUED\r\n")
> DISCARD (expecting "+OK\r\n")
> GET foo (expecting "$-1\r\n" as the response)
> DISCARD (expecting "-ERR DISCARD without MULTI\r\n" as the response)
```

## Failures within transactions

### Your Task

 In this stage, you'll add support for handling failures within a transaction.

### Failures within transactions

When executing a transaction, if a command fails, the error is captured and returned within the response for `EXEC`. All other commands in the transaction are still executed.

You can read more about this behaviour [in the official Redis docs](https://redis.io/docs/latest/develop/interact/transactions/#errors-inside-a-transaction).

Example:

```bash
$ redis-cli
> MULTI
OK
> SET foo xyz
QUEUED
> INCR foo
QUEUED
> SET bar 7
> EXEC
1) OK
2) (error) ERR value is not an integer or out of range
3) OK
```

In the example above, note that the error for the `INCR` command was returned as the second array element. The third command (`SET bar 7`) was still executed successfully.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as a Redis client, and send multiple commands using the same connection:

```bash
$ redis-cli
> SET foo abc
OK
> SET bar 41
OK
> MULTI
OK
> INCR foo
QUEUED
> INCR bar
QUEUED
> EXEC
1) (error) ERR value is not an integer or out of range
2) (integer) 42
```

The expected response for `EXEC` is a [RESP array](https://redis.io/docs/latest/develop/reference/protocol-spec/#arrays) of the responses of the queued commands. The exact bytes will be:

```bash
*2\r\n-ERR value is not an integer or out of range\r\n:42\r\n
```

The tester will also verify that the last command was successfully executed and that the key `bar` exists:

```bash
$ redis-cli
> GET foo (expecting "$3\r\nabc\r\n" as the response)
> GET bar (expecting "$2\r\n42\r\n" as the response)
```

### Notes

- There are a subset of command failures (like syntax errors) that will cause a transaction to be aborted entirely. We won't cover those in this challenge.

## Multiple transactions

### Your Task

In this stage, you'll add support for multiple concurrent transactions.

### Multiple transactions

There can be multiple transactions open (i.e. `MULTI` has been called, but `EXEC` has not been called yet) at the same time. Each transaction gets its own command queue.

For example, say you started transaction 1 from one connection:

```bash
$ redis-cli
> MULTI
OK
> SET foo 41
QUEUED
> INCR foo
QUEUED
```

and started transaction 2 from another connection:

```bash
$ redis-cli
> MULTI
OK
> INCR foo
QUEUED
```

If you then run `EXEC` in transaction 1, you should see the following:

```bash
> EXEC
1) OK
2) (integer) 42
```

`OK` is the response to `SET foo 41`, and `42` is the response to `INCR foo`.

And for transaction 2, running `EXEC` should return:

```bash
> EXEC
1) (integer) 43
```

43 is the response to `INCR foo`. The key `foo` was updated to `42` by transaction 1, and `INCR foo` further increments it to `43`.

### Tests

The tester will execute your program like this:

```bash
$ ./your_program.sh
```

The tester will then connect to your server as multiple Redis clients, and send multiple commands from each connection:

```bash
$ redis-cli MULTI
> INCR foo
> EXEC
```