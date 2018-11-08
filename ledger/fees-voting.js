'use strict';
const proposalRecordsKey = 'proposalRecordsKey';
const voteRecordKeyPrefix ='voteRecords_';
const nonceKey ='nonce';
const passRate = 0.7;
const effectiveProposalInterval =15*1000000*60*60*24;
let proposalRecords = {};
let validators = {};


function loadValidators() {
  let result = getValidators();
  assert(result !== false,'getValidators failed');
  validators = result;
  assert(Object.keys(validators).length !==0,'validators is empty');
}


function loadProposalRecords() {
  let result = storageLoad(proposalRecordsKey);
  if (result === false) {
    return false;
  }
  proposalRecords = JSON.parse(result);
  return true;
}

function isValidator(accountId){
  let found =false;
  validators.every(
    function(item){
      if(item[0] ===accountId) {
        found =true;
        return false;
      }
      return true;
    }
  );
  assert(found,accountId +' is not validator');
}

function voteFee(proposalId) {
  let accountId =sender;
  loadValidators();
  isValidator(accountId);
  if(loadProposalRecords() === false){
    throw 'proposal records not exist';
  }
  assert(proposalRecords.hasOwnProperty(proposalId),'Vote proposal(' + proposalId + ') not exist');

  let key = voteRecordKeyPrefix + proposalId;
  if(blockTimestamp>proposalRecords[proposalId].expireTime){
    delete proposalRecords[proposalId];
    storageStore(proposalRecordsKey, JSON.stringify(proposalRecords));
    storageDel(key);     
    return false;  
  }
  
  let proposalRecordBody = {};
  let result = storageLoad(key);
  assert(result !== false,'proposalId('+proposalId+') not exist voteRecords');
  proposalRecordBody = JSON.parse(result);
  assert(!proposalRecordBody.hasOwnProperty(accountId),'Account(' + accountId + ') have voted the proposal(' + proposalId + ')'); 
  
  proposalRecords[proposalId].voteCount +=1;
  proposalRecordBody[accountId] = 1;


  let thredhold =parseInt(Object.keys(validators).length * passRate + 0.5);
  if(proposalRecords[proposalId].voteCount >= thredhold) {
    let output = {};
    output[proposalRecords[proposalId].feeType] = proposalRecords[proposalId].price;
    delete proposalRecords[proposalId];
    storageDel(key);   
    configFee(JSON.stringify(output));
  }
  else {
    storageStore(key,JSON.stringify(proposalRecordBody));
  }  
  storageStore(proposalRecordsKey, JSON.stringify(proposalRecords));
  return true;
}

function proposalFee(feeType,price) {
  let accountId =sender;
  loadValidators();
  isValidator(accountId);

  let result =storageLoad(nonceKey);
  assert(result !==false,'load nonce failed');
  let nonce = parseInt(result);
  nonce+=1;
  let newProposalId =accountId + nonce;
  loadProposalRecords();
  let exist =false;
  Object.keys(proposalRecords).every(
    function(proposalId){
      if(proposalRecords[proposalId].accountId === accountId) {
        exist =true;
        delete proposalRecords[proposalId];
        let key =voteRecordKeyPrefix + proposalId;
        storageDel(key); 
        proposalRecords[newProposalId] = {'accountId':accountId,'proposalId':newProposalId,'feeType':feeType,'price':price,'voteCount':1,'expireTime':blockTimestamp+effectiveProposalInterval };               
        storageStore(proposalRecordsKey,JSON.stringify(proposalRecords));
        let v={};
        v[accountId] =1;
        storageStore(voteRecordKeyPrefix + newProposalId,JSON.stringify(v));
        return false;
      }
      else{
        return true;
      }        
    }
  );

  if (!exist) {
    proposalRecords[newProposalId] = { 'accountId': accountId, 'proposalId': newProposalId, 'feeType': feeType, 'price': price, 'voteCount': 1,'expireTime':blockTimestamp+effectiveProposalInterval };
    storageStore(proposalRecordsKey, JSON.stringify(proposalRecords));
    let v={};
    v[accountId] =1;
    storageStore(voteRecordKeyPrefix + newProposalId,JSON.stringify(v));
  }  

  storageStore(nonceKey,nonce.toString());
}

function queryVote(proposalId) {
  let key =voteRecordKeyPrefix+proposalId;
  let result = storageLoad(key);
  //assert(result !== false,'vote records of proposal(' +proposalId +') are not existed');
  if(result === false){
    result ='vote records of proposal(' +proposalId +')not exist';
  }
  return result;
}

function queryProposal() {  
  let result = storageLoad(proposalRecordsKey);
  //assert(result !== false,'the proposal is not existed');
  if(result === false){
    result ='proposal not exist';
  }
  return result;
}

function main(input) {
  let para = JSON.parse(input);
  if (para.method === 'voteFee') {
    assert(para.params.proposalId !==undefined,'params proposalId undefined');
    voteFee(para.params.proposalId);
  }
  else if (para.method === 'proposalFee') {
    assert(para.params.feeType !==undefined && para.params.price !==undefined,'params feeType price undefined');
    assert(Number.isInteger(para.params.feeType) && para.params.feeType>0 && para.params.feeType<3,'feeType error');
    assert(Number.isSafeInteger(para.params.price) && para.params.price>=0,'price should be int type and price>=0');
    proposalFee(para.params.feeType,para.params.price);
  }
  else {
    throw 'main input para error';
  }
}

function query(input) {
  let para = JSON.parse(input);
  if (para.method === 'queryVote') {    
    assert(para.params.proposalId !==undefined ,'params.proposalId undefined');
    return queryVote(para.params.proposalId);
  }
  else if (para.method === 'queryProposal') {
    return queryProposal();
  }
  else {
    throw 'query input para error';
  }
}

function init(){ storageStore(nonceKey,'0');}
