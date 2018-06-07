#include "Factory.h"

typedef struct{
    Factory* factory;
    Product* products;
    int num_products;
    unsigned int id;
} produce_t;

typedef struct{
    Factory* factory;
    unsigned int id;
} simpleBuyer_t;

typedef struct{
    Factory* factory;
    int num_products;
    int min_value;
    unsigned int id;
} companyBuyer_t;

typedef struct{
    Factory* factory;
    int num_products;
    unsigned int id;
} thief_t;


void* tryBuyOneWrapper(void* arg){
    simpleBuyer_t* arg_t = (simpleBuyer_t*) arg;
    int* res = new int; //TODO: delete somewhere (deleted)
    *res = arg_t->factory->tryBuyOne();
    return res;
}

void* produceWrapper(void* arg){
    produce_t* arg_t = (produce_t*) arg;
    arg_t->factory->produce(arg_t->num_products, arg_t->products);
}

void* companyBuyReturnWrapper(void* arg){
    companyBuyer_t* arg_t = (companyBuyer_t*) arg;
    std::list<Product> boughtProducts = arg_t->factory->buyProducts(arg_t->num_products);
    std::list<Product> toReturn = boughtProducts;
    bool good_value (const int& value) { return (value >= arg_t->min_value); }
    toReturn.remove_if(good_value);
    int* num_returned = new int; //TODO: delete somewhere (deleted)
    *num_returned = toReturn.size();
    arg_t->factory->returnProducts(toReturn, arg_t->id);
    return num_returned;
}

void* stealProductsWrapper(void* arg){
    thief_t* arg_t = (thief_t*) arg;
    int* res = new int; //TODO: delete somewhere (deleted)
    *res = arg_t->factory->stealProducts(arg_t->num_products, arg_t->id);
    return res;
}

Factory::Factory() : returningServiceOpen(true), factoryOpen(true){
    productsQ = new std::queue<Product>();
    thiefThreads = new std::map<int, pthread_t>();
    companyThreads = new std::map<int, pthread_t>();
    simpleBuyerThreads = new std::map<int, pthread_t>();
    productionThreads = new std::map<int, pthread_t>();
    stolenProducts = new std::list<std::pair<Product, int>>();
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&productsQLock, &mutexattr);
    pthread_mutex_init(&stolenProductsLock, &mutexattr);

    pthread_cond_init(&buy_condition, NULL);
    pthread_cond_init(&return_condition, NULL);
}

Factory::~Factory(){
    pthread_mutex_destroy(&stolenProductsLock);
    pthread_mutex_destroy(&productsQLock);
    delete(stolenProducts);
    delete(productionThreads);
    delete(simpleBuyerThreads);
    delete(companyThreads);
    delete(thiefThreads);
    delete(productsQ);
}

void Factory::startProduction(int num_products, Product* products,unsigned int id){
    pthread_t p;

    produce_t* arg = new produce_t; //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->products = products;
    arg->num_products = num_products;
    arg->id = id;
    productionThreads->insert(std::pair<int, pthread_t>(id, p)); //TODO: check if should be after next line
    pthread_create(&p, NULL, produceWrapper, arg);
}

void Factory::produce(int num_products, Product* products){
    pthread_mutex_lock(&productsQLock);
    for(int i=0;i<num_products;i++)
        productsQ->push(products[i]);
    pthread_mutex_unlock(&productsQLock);
}

void Factory::finishProduction(unsigned int id){
    pthread_t thread = productionThreads->at(id);
    pthread_join(thread, NULL);
}

void Factory::startSimpleBuyer(unsigned int id){
    pthread_t p;

    simpleBuyer_t* arg = new simpleBuyer_t; //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->id = id;
    simpleBuyerThreads->insert(std::pair<int, pthread_t>(id, p)); //TODO: check if should be after next line
    pthread_create(&p, NULL, tryBuyOneWrapper, arg);
}

int Factory::tryBuyOne(){
    if(pthread_mutex_trylock(&productsQLock))
        return -1; //factory busy
    if(productsQ->empty() || !factoryOpen){
        pthread_mutex_unlock(&productsQLock);
        return -1;
    }
    int id = productsQ->front().getId();
    productsQ->pop(); //assuming front deosnt remove the element
    pthread_mutex_unlock(&productsQLock);
    return id;
}

int Factory::finishSimpleBuyer(unsigned int id){
    pthread_t thread = simpleBuyerThreads->at(id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete pval; //TODO: check
    return res;
}

void Factory::startCompanyBuyer(int num_products, int min_value,unsigned int id){
    pthread_t p;

    companyBuyer_t* arg = new companyBuyer_t; //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->num_products = num_products;
    arg->min_value = min_value;
    arg->id = id;
    companyThreads->insert(std::pair<int, pthread_t>(id, p)); //TODO: check if should be after next line
    pthread_create(&p, NULL, companyBuyReturnWrapper, arg);
}

std::list<Product> Factory::buyProducts(int num_products){
    std::list<Product> boughtList;
    Product p;
    pthread_mutex_lock(&productsQLock);

    while(num_products>productsQ->size() || !thiefThreads->empty() || !factoryOpen){
        pthread_cond_wait(&buy_condition , &productsQLock);
    }
    while(num_products){
        p = productsQ->front();
        boughtList.push_front(p); //TODO: check order is fine (according to return function)
        productsQ->pop();
        num_products--;
    }

    pthread_mutex_unlock(&productsQLock);
    return boughtList;
}

void Factory::returnProducts(std::list<Product> products,unsigned int id){
    pthread_mutex_lock(&productsQLock);

    while(!thiefThreads->empty() || !factoryOpen || !returningServiceOpen){
        pthread_cond_wait(&return_condition , &productsQLock);
    }
    for(Product p: products){
        productsQ->push(p);
    }

    pthread_cond_broadcast(&buy_condition); //TODO: check if signal
    //TODO: check if "add signal"

    pthread_mutex_unlock(&productsQLock);
    return;
}

int Factory::finishCompanyBuyer(unsigned int id){
    pthread_t thread = simpleBuyerThreads->at(id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete pval; //TODO: check
    return res;
}

void Factory::startThief(int num_products,unsigned int fake_id){
    pthread_t p;

    thief_t* arg = new thief_t; //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->num_products = num_products;
    arg->id = fake_id;
    thiefThreads->insert(std::pair<int, pthread_t>(fake_id, p)); //TODO: check if should be after next line
    pthread_create(&p, NULL, stealProductsWrapper, arg);
}

int Factory::stealProducts(int num_products,unsigned int fake_id){
    if(/*TODO: check here if he cant access*/)
        wait;
    int num_stolen=0;
    while(!productsQ->empty() && num_stolen<num_products){
        //TODO: complete
    }
    return num_stolen;
}

int Factory::finishThief(unsigned int fake_id){
    pthread_t thread = thiefThreads->at(fake_id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete pval; //TODO: check
    return res;
}

void Factory::closeFactory(){
    //TODO: complete
}

void Factory::openFactory(){
    //TODO: complete
}

void Factory::closeReturningService(){
    //TODO: complete
}

void Factory::openReturningService(){
    //TODO: complete
}

std::list<std::pair<Product, int>> Factory::listStolenProducts(){
    //TODO: complete
    return std::list<std::pair<Product, int>>();
}

std::list<Product> Factory::listAvailableProducts(){
    //TODO: complete
    return std::list<Product>();
}


std::map<int, pthread_t> *Factory::getThiefThreads() const {
    return thiefThreads;
}

std::map<int, pthread_t> *Factory::getCompanyThreads() const {
    return companyThreads;
}

std::map<int, pthread_t> *Factory::getSimpleBuyerThreads() const {
    return simpleBuyerThreads;
}

std::map<int, pthread_t> *Factory::getProductionThreads() const {
    return productionThreads;
}
