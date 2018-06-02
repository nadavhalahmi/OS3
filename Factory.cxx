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
    arg_t->factory->tryBuyOne();
}

void* produceWrapper(void* arg){
    produce_t* arg_t = (produce_t*) arg;
    arg_t->factory->produce(arg_t->num_products, arg_t->products);
}

void* companyBuyReturnWrapper(void* arg){
    companyBuyer_t* arg_t = (companyBuyer_t*) arg;
    std::list<Product> boughtProducts = arg_t->factory->buyProducts(arg_t->num_products);
    arg_t->factory->returnProducts(boughtProducts, arg_t->id);
}

void* stealProductsWrapper(void* arg){
    thief_t* arg_t = (thief_t*) arg;
    arg_t->factory->stealProducts(arg_t->num_products, arg_t->id);
}

Factory::Factory() : returningServiceOpen(true), factoryOpen(true){
    productsQ = new std::queue<Product>();
    thiefThreads = new std::map<int, pthread_t>();
    companyThreads = new std::map<int, pthread_t>();
    simpleBuyerThreads = new std::map<int, pthread_t>();
    productionThreads = new std::map<int, pthread_t>();
}

Factory::~Factory(){
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
    //TODO: maybe lock productsQ
    for(int i=0;i<num_products;i++)
        productsQ->push(products[i]);
}

void Factory::finishProduction(unsigned int id){
    pthread_t thread = productionThreads->at(id);
    //TODO: complete
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
    //TODO: maybe lock productsQ
    if(productsQ->empty())
        return -1;
    if(/*TODO: check here if a thread is working on the factory at the moment*/)
        return -1;
    int id = productsQ->front().getId();
    productsQ->pop(); //assuming front deosnt remove the element
    return id;
}

int Factory::finishSimpleBuyer(unsigned int id){
    //TODO: complete and check whtat about the return value (should tryBuyOne be called?)
    return -1;
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
    if(/*TODO: check if it cant access*/)
        wait;
    //TODO: check what if num_products is too big
    //TODO: complete
    return std::list<Product>();
}

void Factory::returnProducts(std::list<Product> products,unsigned int id){
    //TODO: complete
}

int Factory::finishCompanyBuyer(unsigned int id){
    //TODO: complete
    return 0;
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
    //TODO: complete
    return 0;
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
