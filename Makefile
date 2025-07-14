start:
	mkdir -p build

	kubectl kustomize farm_1/ > build/farm_1.yaml
	kubectl apply -f build/farm_1.yaml

	kubectl kustomize farm_2/ > build/farm_2.yaml
	kubectl apply -f build/farm_2.yaml

stop:
	kubectl delete -f build/farm_1.yaml
	kubectl delete -f build/farm_2.yaml

images:
	docker build -t my-adproxy:v1 -f adproxy.dockerfile .
	docker build -t my-adserver:v1 -f adserver.dockerfile .
	docker build -t my-daemon:v1 -f daemon.dockerfile .
	docker build -t my-database:v1 -f database.dockerfile .

build-local:
	mkdir -p build
	cd build && cmake .. && make -j4

test-local: build-local
	./build/adserver
