# Task for Virtual Minds candidates

Welcome to the Adserver Team challenge!

In this task we are trying to access the following qualities:
- Ability to learn. You would probably have to google a few times.
- Basic knowledge of Kubernetes. The task includes minor updating of a YAML file.
- C++ skills. You would need to work with `boost::asio`.
- Operating systems basic skills. If you know where to optimize and how.
- Code and architecture design. How customizable the code is,
  how flexible and fast is the system.

It probably requires 2-8 hours of your time, depending on previous experience and how
optimal and flexible your solution is going to be.

## Task context

You are given a simple Kubernetes cluster config. It consists of farms. The farms are
identical except for their minor config differences and which customers they serve. One
could say the farms are a sort of sharding. You can find the farm template in `farm_base/`
folder.

A farm has the following:

### Adserver
**Files**: `farm_base/adserver-deployment.yaml`, `adserver.py`.
The main workers of the entire system. Adservers serve banners and can do some
simple meta requests like returning their node info. Adservers operate entirely via HTTP
protocol, like everything else in this task.

### Adproxy
**Files**: `farm_base/adproxy-deployment.yaml`, `adproxy.py`.
The adservers are accessed via a proxy. The proxies implement a special routing logic. For
example, the HTTP query parameter `host=hostname` would make the request be forwarded to
the adserver Pod having the given `hostname`.

To support that routing logic the individual adserver Pods are exposed using a Headless
Kubernetes service (see `ad-svc` Service in `farm_base/adserver-deployment.yaml`).

The proxies are exposed to the open internet via a LoadBalancer Kubernetes service (see
`adproxy-loadbalancer` in `farm_base/adproxy-deployment.yaml`).

### Database
**Files**: `farm_base/database-deployment.yaml`, `database.py`.
The banners are "stored" in a DB, which is basically a Python dictionary without any
persistence, but it has a key-value HTTP API, like "get" and "set".

### Daemon
**Files**: `farm_base/daemon-deployment.yaml`, `daemon.py`.
The DB can't be accessed directly. Instead, users must do it via `daemon` service. It
builds proper queries and sends them to the DB. The adservers, in turn, must fetch the
banners from the DB.

### Static data
**Files**: `farm_base/geodata-pv.yaml`, `geodata/` folder.
Besides, the adservers need geodata for targeting and filtering. This data changes very
rarely and is stored simply on disk as files. However since it is not related to any
specific farm or server, the data is shared by the whole cluster. The same data is mounted
to all the adserver Pods in all farms.

## Task itself

Almost everything is already in place. The task's scope is entirely just the `adserver`.
1. Add the missing fields in `farm_base/adserver-deployment.yaml`. Look for `???` in it.
2. Implement `adserver.cpp` using `boost::asio` for networking. For HTTP you can use
  `boost::beast`, but that isn't required. `adserver.py` must be dropped.
3. Make `adserver.dockerfile` build your solution into a container image, to be used by
   Kubernetes.

As a working reference implementation there is `adserver.py`. It already works with the
current `adserver.dockerfile`. You can use it for testing if your
`farm_base/adserver-deployment.yaml` is correct, after you do the step-1, before
continuing.

While programming, imagine that your solution is going to experience high load in
production. Make design choices based on that.

## Recommendations

It is recommended to firstly fix `adserver-deployment.yaml` and test if it works with the
existing `adserver.py` and `adserver.dockerfile`, using the commands below.

Once this is settled, it would be convenient to implement just a simple `/` HTTP request
in `adserver.cpp`, make it build successfully with `adserver.dockerfile`, and test if this
HTTP path works.

Now you only have to support more paths, with the build and deploy pipelines already
working.

# Testing

Here is how you might test your solution.

## Start minikube cluster
```Bash
minikube start
minikube mount geodata:/mnt/adserver_geodata &
```

## Build the images
```Bash
eval $(minikube docker-env)
make images
```

## Start the cluster
```Bash
make start
```

## Talk to it
The following `curl` commands must remain working with your implementation.

```Bash
# Find the cluster's frontend IPs and ports.
eval "$(./find_farms.sh)"

# Make some random requests.
curl "$ad_adition_farm1/"
curl "$ad_adition_farm2/"

# Check routing.
# Must go to adserver-0 in farm1.
curl "$ad_adition_farm1/?host=adserver-0"
# Must go to adserver-2 in farm2.
curl "$ad_adition_farm2/?host=adserver-2"

# Check the config.
curl "$ad_adition_farm1/cfg"
curl "$ad_adition_farm2/cfg"

# Create a banner.
curl "$dm_adition_farm1/new-banner?name=test&data=some-data-farm1"
curl "$dm_adition_farm2/new-banner?name=test&data=some-data-farm2"
# Deliver the banner.
curl "$ad_adition_farm1/banner?userID=123&name=test"
curl "$ad_adition_farm2/banner?userID=123&name=test"

# Another banner is not found, created, then found.
curl "$ad_adition_farm1/banner?userID=123&name=test2"
curl "$dm_adition_farm1/new-banner?name=test2&data=other-data"
curl "$ad_adition_farm1/banner?userID=123&name=test2"

# Check that the geo data is available.
curl "$ad_adition_farm1/geo"
curl "$ad_adition_farm2/geo"
curl "$ad_adition_farm1/plz"
curl "$ad_adition_farm2/plz"
```
