#ifndef FACTORY_H_
#define FACTORY_H_

#include <pthread.h>
#include <list>
#include <queue>
#include <map>
#include "Product.h"

class Factory{
private:
    bool returningServiceOpen;
    bool factoryOpen;
    std::deque<Product> *productsQ;
    //TODO: check if should be pthread_t*
    std::map<int, pthread_t> *thiefThreads;
    std::map<int, pthread_t> *companyThreads;
    std::map<int, pthread_t> *simpleBuyerThreads;
    std::map<int, pthread_t> *productionThreads;
    std::list<std::pair<Product, int>>* stolenProducts;
    pthread_mutex_t productsQLock;
    pthread_mutex_t stolenProductsLock;

    pthread_cond_t buy_condition;
    pthread_cond_t return_condition;
    pthread_cond_t thief_condition;
public:
    Factory();
    ~Factory();

    void startProduction(int num_products, Product* products, unsigned int id);
    void produce(int num_products, Product* products);
    void finishProduction(unsigned int id);
    
    void startSimpleBuyer(unsigned int id);
    int tryBuyOne();
    int finishSimpleBuyer(unsigned int id);

    void startCompanyBuyer(int num_products, int min_value,unsigned int id);
    std::list<Product> buyProducts(int num_products);
    void returnProducts(std::list<Product> products,unsigned int id);
    int finishCompanyBuyer(unsigned int id);

    void startThief(int num_products,unsigned int fake_id);
    int stealProducts(int num_products,unsigned int fake_id);
    int finishThief(unsigned int fake_id);

    void closeFactory();
    void openFactory();
    
    void closeReturningService();
    void openReturningService();
    
    std::list<std::pair<Product, int>> listStolenProducts();
    std::list<Product> listAvailableProducts();

};
#endif // FACTORY_H_
