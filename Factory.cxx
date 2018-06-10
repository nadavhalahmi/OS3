#include <iostream>
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
    int* res = new int();
    *res = arg_t->factory->tryBuyOne();
    delete arg_t;
    return res;
}

void* produceWrapper(void* arg){
    produce_t* arg_t = (produce_t*) arg;
    arg_t->factory->produce(arg_t->num_products, arg_t->products);
    delete arg_t;
    return NULL;
}

void* companyBuyReturnWrapper(void* arg){
    companyBuyer_t* arg_t = (companyBuyer_t*) arg;
    std::list<Product> boughtProducts = arg_t->factory->buyProducts(arg_t->num_products);
    std::list<Product> toReturn; //check new is missing is fine
    for(Product p : boughtProducts){
        if(p.getValue() < arg_t->min_value)
            toReturn.push_back(p); //check order
    }
    int* num_returned = new int();
    *num_returned = toReturn.size();
    arg_t->factory->returnProducts(toReturn, arg_t->id);
    delete arg_t;
    return num_returned;
}

void* stealProductsWrapper(void* arg){
    thief_t* arg_t = (thief_t*) arg;
    int* res = new int();
    *res = arg_t->factory->stealProducts(arg_t->num_products, arg_t->id);
    delete arg_t;
    return res;
}

Factory::Factory() : returningServiceOpen(true), factoryOpen(true), activeThiefs(0){
    productsQ = new std::deque<Product>();
    thiefThreads = new std::map<int, pthread_t>();
    companyThreads = new std::map<int, pthread_t>();
    simpleBuyerThreads = new std::map<int, pthread_t>();
    productionThreads = new std::map<int, pthread_t>();
    stolenProducts = new std::list<std::pair<Product, int>>();
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&productsQLock, &mutexattr);
    pthread_mutex_init(&stolenProductsLock, &mutexattr);

    pthread_cond_init(&buy_condition, NULL);
    pthread_cond_init(&return_condition, NULL);
    pthread_cond_init(&thief_condition, NULL);
}

Factory::~Factory(){
    pthread_mutex_destroy(&stolenProductsLock);
    pthread_mutex_destroy(&productsQLock);
    pthread_mutexattr_destroy(&mutexattr);
    delete(stolenProducts);
    delete(productionThreads);
    delete(simpleBuyerThreads);
    delete(companyThreads);
    delete(thiefThreads);
    delete(productsQ);
}

void Factory::startProduction(int num_products, Product* products,unsigned int id){
    pthread_t p;

    produce_t* arg = new produce_t();
    arg->factory = this;
    arg->products = products;
    arg->num_products = num_products;
    arg->id = id;
    pthread_create(&p, NULL, produceWrapper, arg);
    productionThreads->insert(std::pair<int, pthread_t>(id, p));
}

void Factory::produce(int num_products, Product* products){
    pthread_mutex_lock(&productsQLock);
    for(int i=0;i<num_products;i++)
        productsQ->push_back(products[i]);

    //signal that we added products
    pthread_cond_signal(&thief_condition);
    pthread_cond_broadcast(&buy_condition);
    pthread_mutex_unlock(&productsQLock);
}

void Factory::finishProduction(unsigned int id){
    pthread_t thread = productionThreads->at(id);
    pthread_join(thread, NULL);
    productionThreads->erase(id);
}

void Factory::startSimpleBuyer(unsigned int id){
    pthread_t p;

    simpleBuyer_t* arg = new simpleBuyer_t();
    arg->factory = this;
    arg->id = id;
    pthread_create(&p, NULL, tryBuyOneWrapper, arg);
    simpleBuyerThreads->insert(std::pair<int, pthread_t>(id, p));
}

int Factory::tryBuyOne(){
    if(pthread_mutex_trylock(&productsQLock))
        return -1; //factory busy
    if(productsQ->empty() || !factoryOpen){
        pthread_mutex_unlock(&productsQLock);
        return -1;
    }
    int id = productsQ->front().getId();
    productsQ->pop_front();
    pthread_mutex_unlock(&productsQLock);
    return id;
}

int Factory::finishSimpleBuyer(unsigned int id){
    pthread_t thread = simpleBuyerThreads->at(id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete (int*)(pval);
    simpleBuyerThreads->erase(id);
    return res;
}

void Factory::startCompanyBuyer(int num_products, int min_value,unsigned int id){
    pthread_t p;

    companyBuyer_t* arg = new companyBuyer_t();
    arg->factory = this;
    arg->num_products = num_products;
    arg->min_value = min_value;
    arg->id = id;
    pthread_create(&p, NULL, companyBuyReturnWrapper, arg);
    companyThreads->insert(std::pair<int, pthread_t>(id, p));
}

std::list<Product> Factory::buyProducts(int num_products){
    pthread_mutex_lock(&productsQLock);

    while(num_products>productsQ->size() || activeThiefs || !factoryOpen){
        pthread_cond_wait(&buy_condition , &productsQLock);
    }
    std::list<Product> boughtList;
    Product p;
    while(num_products){
        p = productsQ->front();
        boughtList.push_back(p);
        productsQ->pop_front();
        num_products--;
    }
    pthread_mutex_unlock(&productsQLock);
    return boughtList;
}

void Factory::returnProducts(std::list<Product> products,unsigned int id){
    if(products.empty())
        return;
    pthread_mutex_lock(&productsQLock);
    while(activeThiefs || !factoryOpen || !returningServiceOpen){
        pthread_cond_wait(&return_condition , &productsQLock);
    }
    for(Product p: products){
        productsQ->push_back(p);
    }


    pthread_cond_broadcast(&buy_condition);
    pthread_cond_signal(&return_condition);
    pthread_mutex_unlock(&productsQLock);
    return;
}

int Factory::finishCompanyBuyer(unsigned int id){
    pthread_t thread = companyThreads->at(id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete (int*)(pval);
    companyThreads->erase(id);
    return res;
}

void Factory::startThief(int num_products,unsigned int fake_id){
    pthread_mutex_lock(&productsQLock);
    pthread_t p;

    thief_t* arg = new thief_t();
    arg->factory = this;
    arg->num_products = num_products;
    arg->id = fake_id;
    pthread_create(&p, NULL, stealProductsWrapper, arg);
    thiefThreads->insert(std::pair<int, pthread_t>(fake_id, p));
    activeThiefs++;
    pthread_mutex_unlock(&productsQLock);
}

int Factory::stealProducts(int num_products,unsigned int fake_id){
    pthread_mutex_lock(&productsQLock);

    while(!factoryOpen){
        pthread_cond_wait(&thief_condition, &productsQLock);
    }
    pthread_mutex_lock(&stolenProductsLock);
    Product p;
    int num_stolen=0;
    while(!productsQ->empty() && num_stolen<num_products){
        p = productsQ->front();
        stolenProducts->push_back(std::pair<Product, int>(p, fake_id));
        productsQ->pop_front();
        num_stolen++;
    }
    pthread_mutex_unlock(&stolenProductsLock);

    activeThiefs--;
    pthread_cond_broadcast(&buy_condition);
    pthread_cond_signal(&return_condition);
    pthread_cond_signal(&thief_condition);
    pthread_mutex_unlock(&productsQLock);
    return num_stolen;
}

int Factory::finishThief(unsigned int fake_id){
    pthread_t thread = thiefThreads->at(fake_id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete (int*)(pval);
    thiefThreads->erase(fake_id);
    return res;
}

void Factory::closeFactory(){
    pthread_mutex_lock(&productsQLock);
    factoryOpen = false;
    pthread_mutex_unlock(&productsQLock);
}

void Factory::openFactory(){
    pthread_mutex_lock(&productsQLock);
    factoryOpen = true;
    pthread_cond_broadcast(&buy_condition);
    pthread_cond_signal(&return_condition);
    pthread_cond_signal(&thief_condition);
    pthread_mutex_unlock(&productsQLock);
}

void Factory::closeReturningService(){
    pthread_mutex_lock(&productsQLock);
    returningServiceOpen = false;
    pthread_mutex_unlock(&productsQLock);
}

void Factory::openReturningService(){
    pthread_mutex_lock(&productsQLock);
    returningServiceOpen = true;
    pthread_cond_signal(&return_condition);
    pthread_mutex_unlock(&productsQLock);
}

std::list<std::pair<Product, int>> Factory::listStolenProducts(){
    pthread_mutex_lock(&stolenProductsLock);
    std::list<std::pair<Product, int>> res =  *stolenProducts;
    pthread_mutex_unlock(&stolenProductsLock);
    return res;
}

std::list<Product> Factory::listAvailableProducts(){
    pthread_mutex_lock(&productsQLock);
    std::list<Product> productsList;
    for(Product p : *productsQ){
        productsList.push_back(p);
    }
    pthread_mutex_unlock(&productsQLock);
    return productsList;
}