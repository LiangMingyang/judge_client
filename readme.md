
# install

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

echo "deb https://apt.dockerproject.org/repo ubuntu-precise main" > /etc/apt/sources.list.d/docker.list

apt-get update

apt-get install docker-engine -y

git clone https://github.com/LiangMingyang/judge_client.git

docker build -t oj4th/judge_container judge_client/container/

```