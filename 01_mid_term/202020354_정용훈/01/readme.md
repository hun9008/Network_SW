# ENV

| OS | compiler |
| --- | --- |
| M3 Pro Mac | CLang |

```
// compile

gcc ManagerServer.c -o managerserver
gcc MulticastSender.c -o multicastsender
```

```
// run

./managerserver 9000
./multicastsender 127.0.0.1 9000
```


