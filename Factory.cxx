#include "Factory.h"

typedef struct{
    Factory* factory;
    Product* products;
    int num_products;
} produce_t;



void* produceWrapper(void* arg){
    produce_t* arg_t = (produce_t*) arg;
    arg_t->factory->produce(arg_t->num_products, arg_t->products);
}


Factory::Factory() : returningServiceOpen(true), factoryOpen(true){
    productsQ = new std::queue<Product>();
}

Factory::~Factory(){
    delete(productsQ);
}

void Factory::startProduction(int num_products, Product* products,unsigned int id){
    //TODO: check what to do with id (maybe add to collection or something)
    pthread_t p;

    produce_t* arg = new produce_t; //TODO: delete somewhere and keep arg somewhere to delete
    arg->factory = this;
    arg->products = products;
    arg->num_products = num_products;
    pthread_create(&p, NULL, produceWrapper, arg);
}

void Factory::produce(int num_products, Product* products){
    for(int i=0;i<num_products;i++)
        productsQ->push(products[i]);
}

void Factory::finishProduction(unsigned int id){
}

void Factory::startSimpleBuyer(unsigned int id){
}

int Factory::tryBuyOne(){
    return -1;
}

int Factory::finishSimpleBuyer(unsigned int id){
    return -1;
}

void Factory::startCompanyBuyer(int num_products, int min_value,unsigned int id){
}

std::list<Product> Factory::buyProducts(int num_products){
    return std::list<Product>();
}

void Factory::returnProducts(std::list<Product> products,unsigned int id){
}

int Factory::finishCompanyBuyer(unsigned int id){
    return 0;
}

void Factory::startThief(int num_products,unsigned int fake_id){
}

int Factory::stealProducts(int num_products,unsigned int fake_id){
    return 0;
}

int Factory::finishThief(unsigned int fake_id){
    return 0;
}

void Factory::closeFactory(){
}

void Factory::openFactory(){
}

void Factory::closeReturningService(){
}

void Factory::openReturningService(){
}

std::list<std::pair<Product, int>> Factory::listStolenProducts(){
    return std::list<std::pair<Product, int>>();
}

std::list<Product> Factory::listAvailableProducts(){
    return std::list<Product>();
}
