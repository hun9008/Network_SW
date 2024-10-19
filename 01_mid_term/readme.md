# port 9000~9010까지 강제 종료

```
for port in {9000..9010}; do sudo lsof -t -i tcp:$port | xargs -r sudo kill -9; done
```