
# ubuntu_install

```

apt-get update

apt-get upgrade -y

apt-get install build-essential -y

apt-get install git -y

apt-get install curl -y

curl -sL https://deb.nodesource.com/setup_4.x | sudo -E bash -

apt-get update

apt-get install -y nodejs -y

apt-key adv --keyserver hkp://p80.pool.sks-keyservers.net:80 --recv-keys 58118E89F3A912897C070ADBF76221572C52609D

| Ubuntu version | Repository |
| :-------- | --------:| :--: |
| Precise 12.04 (LTS)	| deb https://apt.dockerproject.org/repo ubuntu-precise main|
| Trusty 14.04 (LTS)	| deb https://apt.dockerproject.org/repo ubuntu-trusty main |
| Wily 15.10	| deb https://apt.dockerproject.org/repo ubuntu-wily main|
| Xenial 16.04 (LTS)	| deb https://apt.dockerproject.org/repo ubuntu-xenial main|

echo "deb https://apt.dockerproject.org/repo ubuntu-trusty main" > /etc/apt/sources.list.d/docker.list

apt-get update

apt-get install docker-engine -y

git clone https://github.com/LiangMingyang/judge_client.git

docker build -t oj4th/judge_container judge_client/container/

```

# windows_install

安装node.js环境

[下载指定windows docker](https://download.docker.com/win/stable/13194/Docker%20for%20Windows%20Installer.exe)

安装之后运行

docker build -t oj4th/judge_container judge_client/container/

# run

配置config.js，配置服务器地址等

然后node connector.js start

