#include <iostream>
#include "Factory.h"

template class std::map<int, pthread_t>; //TODO: delete this line after debugging

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
    int* res = new int;
    *res = arg_t->factory->tryBuyOne();
    delete arg_t; //TODO: check arg stuff aren't deleted (factory for example)
    return res;
}

void* produceWrapper(void* arg){
    produce_t* arg_t = (produce_t*) arg;
    arg_t->factory->produce(arg_t->num_products, arg_t->products);
    delete arg_t; //TODO: check arg stuff aren't deleted (factory for example)
}

void* companyBuyReturnWrapper(void* arg){
    companyBuyer_t* arg_t = (companyBuyer_t*) arg;
    std::list<Product> boughtProducts = arg_t->factory->buyProducts(arg_t->num_products);
    std::list<Product> toReturn; //check new is missing is fine
    for(Product p : toReturn){
        if(p.getValue() < arg_t->min_value)
            toReturn.push_back(p); //check order
    }
    int* num_returned = new int; //TODO: delete somewhere (deleted)
    *num_returned = toReturn.size();
    arg_t->factory->returnProducts(toReturn, arg_t->id);
    delete arg_t; //TODO: check arg stuff aren't deleted (factory for example)
    return num_returned;
}

void* stealProductsWrapper(void* arg){
    thief_t* arg_t = (thief_t*) arg;
    int* res = new int; //TODO: delete somewhere (deleted)
    *res = arg_t->factory->stealProducts(arg_t->num_products, arg_t->id);
    delete arg_t; //TODO: check arg stuff aren't deleted (factory for example)
    return res;
}

Factory::Factory() : returningServiceOpen(true), factoryOpen(true){
    productsQ = new std::deque<Product>();
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
    pthread_cond_init(&thief_condition, NULL);
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

    produce_t* arg = new produce_t(); //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->products = products;
    arg->num_products = num_products;
    arg->id = id;
    pthread_create(&p, NULL, produceWrapper, arg);
    productionThreads->insert(std::pair<int, pthread_t>(id, p));
}

void Factory::produce(int num_products, Product* products){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in produce\n";
    for(int i=0;i<num_products;i++)
        productsQ->push_back(products[i]);
    pthread_cond_broadcast(&buy_condition);
    std::cout << "thread with id " << pthread_self() << " released the lock in produce\n";
    pthread_mutex_unlock(&productsQLock);
}

void Factory::finishProduction(unsigned int id){
    pthread_t thread = productionThreads->at(id);
    pthread_join(thread, NULL);
}

void Factory::startSimpleBuyer(unsigned int id){
    pthread_t p;

    simpleBuyer_t* arg = new simpleBuyer_t(); //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->id = id;
    pthread_create(&p, NULL, tryBuyOneWrapper, arg);
    simpleBuyerThreads->insert(std::pair<int, pthread_t>(id, p)); //TODO: check if should be after next line
}

int Factory::tryBuyOne(){
    if(pthread_mutex_trylock(&productsQLock))
        return -1; //factory busy
    std::cout << "thread with id " << pthread_self() << " got the lock in tryBuyOne\n";
    if(productsQ->empty() || !factoryOpen){
        std::cout << "thread with id " << pthread_self() << " released the lock in tryBuyOne\n";
        pthread_mutex_unlock(&productsQLock);
        return -1;
    }
    int id = productsQ->front().getId();
    productsQ->pop_front(); //assuming front deosnt remove the element
    std::cout << "thread with id " << pthread_self() << " released the lock in tryBuyOne\n";
    pthread_mutex_unlock(&productsQLock);
    return id;
}

int Factory::finishSimpleBuyer(unsigned int id){
    pthread_t thread = simpleBuyerThreads->at(id);
    void* pval;
    int debug = pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete (int*)(pval); //TODO: check
    simpleBuyerThreads->erase(id); //TODO: check why this erase is here while others are not
    return res;
}

void Factory::startCompanyBuyer(int num_products, int min_value,unsigned int id){
    pthread_t p;

    companyBuyer_t* arg = new companyBuyer_t(); //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->num_products = num_products;
    arg->min_value = min_value;
    arg->id = id;
    pthread_create(&p, NULL, companyBuyReturnWrapper, arg);
    companyThreads->insert(std::pair<int, pthread_t>(id, p));
}

std::list<Product> Factory::buyProducts(int num_products){
    std::list<Product> boughtList;
    Product p;
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in buyProducts\n";

    while(num_products>productsQ->size() || !thiefThreads->empty() || !factoryOpen){
        std::cout << "wait: (buyProducts) buy_condition" << std::endl;
        pthread_cond_wait(&buy_condition , &productsQLock);
    }
    while(num_products){
        p = productsQ->front();
        boughtList.push_front(p); //TODO: check order is fine (according to return function)
        productsQ->pop_front();
        num_products--;
    }
    std::cout << "thread with id " << pthread_self() << " released the lock in buyProducts\n";
    pthread_mutex_unlock(&productsQLock);
    return boughtList;
}

void Factory::returnProducts(std::list<Product> products,unsigned int id){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in returnProducts\n";

    while(!thiefThreads->empty() || !factoryOpen || !returningServiceOpen){
        std::cout << "wait: (returnProducts) return_condition" << std::endl;
        pthread_cond_wait(&return_condition , &productsQLock);
    }
    for(Product p: products){
        productsQ->push_back(p);
    }

    pthread_cond_broadcast(&buy_condition); //TODO: check if signal
    //TODO: check if "add signal"
    std::cout << "thread with id " << pthread_self() << " released the lock in returnProducts\n";
    pthread_mutex_unlock(&productsQLock);
    return;
}

int Factory::finishCompanyBuyer(unsigned int id){
    pthread_t thread = companyThreads->at(id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete (int*)(pval); //TODO: check
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
    pthread_mutex_unlock(&productsQLock);
}

int Factory::stealProducts(int num_products,unsigned int fake_id){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in stealProducts\n";

    while(!factoryOpen){
        std::cout << "wait: (stealProducts) thief_condition" << std::endl;
        pthread_cond_wait(&thief_condition, &productsQLock);
    }
   //pthread_mutex_lock(&stolenProductsLock);
    Product p;
    int num_stolen=0;
    while(!productsQ->empty() && num_stolen<num_products){
        p = productsQ->front();
        stolenProducts->push_back(std::pair<Product, int>(p, fake_id)); //oreder should be good, check anyway
        productsQ->pop_front();
        num_stolen++;
    }
    //pthread_mutex_unlock(&stolenProductsLock);
    pthread_cond_broadcast(&buy_condition);
    pthread_cond_broadcast(&return_condition);
    pthread_cond_broadcast(&thief_condition);
    std::cout << "thread with id " << pthread_self() << " released the lock in stealProducts\n";
    pthread_mutex_unlock(&productsQLock); //TODO: check unlock timing
    return num_stolen;
}

int Factory::finishThief(unsigned int fake_id){
    pthread_t thread = thiefThreads->at(fake_id);
    void* pval;
    pthread_join(thread, &pval);
    int res = *(int*)(pval);
    delete (int*)(pval); //TODO: check
    thiefThreads->erase(fake_id);
    return res;
}

void Factory::closeFactory(){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in closeFactory\n";
    factoryOpen = false;
    std::cout << "thread with id " << pthread_self() << " released the lock in closeFactory\n";
    pthread_mutex_unlock(&productsQLock);
}

void Factory::openFactory(){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in openFactory\n";
    factoryOpen = true;
    pthread_cond_broadcast(&buy_condition);
    pthread_cond_broadcast(&return_condition);//check if "returnService" needs to be checked
    pthread_cond_broadcast(&thief_condition);
    std::cout << "thread with id " << pthread_self() << " released the lock in openFactory\n";
    pthread_mutex_unlock(&productsQLock);
}

void Factory::closeReturningService(){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in closeReturningService\n";
    returningServiceOpen = false;
    std::cout << "thread with id " << pthread_self() << " released the lock in closeReturningService\n";
    pthread_mutex_unlock(&productsQLock);
}

void Factory::openReturningService(){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in openReturningService\n";
    returningServiceOpen = true;
    pthread_cond_broadcast(&return_condition);//check if "factoryOpen" needs to be checked
    std::cout << "thread with id " << pthread_self() << " released the lock in openReturningService\n";
    pthread_mutex_unlock(&productsQLock);
}

std::list<std::pair<Product, int>> Factory::listStolenProducts(){
    //lock stolen products
    return *stolenProducts;
}

std::list<Product> Factory::listAvailableProducts(){
    pthread_mutex_lock(&productsQLock);
    std::cout << "thread with id " << pthread_self() << " got the lock in listAvailableProducts\n";
    std::list<Product> productsList;
    for(Product p : *productsQ){
        productsList.push_back(p);
    }
    std::cout << "thread with id " << pthread_self() << " released the lock in listAvailableProducts\n";
    pthread_mutex_unlock(&productsQLock);
    return productsList;
}
