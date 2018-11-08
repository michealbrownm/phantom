'use strict';

const validatorSetSize       = 100;
const passRate               = 0.7;
const effectiveVoteInterval  = 15 * 24 * 60 * 60 * 1000 * 1000;
const minPledgeAmount        = 50000 * 100000000;
const minSuperadditionAmount = 100 * 100000000;
const applicantVar    = 'applicant_';
const abolishVar      = 'abolish_';
const proposerVar     = 'proposer';
const reasonVar       = 'reason';
const ballotVar       = 'ballot';
const candidatesVar   = 'validator_candidates';
const pledgeAmountVar = 'pledge_coin_amount';
const expiredTimeVar  = 'voting_expired_time';

function byString(name){
    let fun = function(x,y){
        assert(x && y && typeof x === 'object' && typeof y ==='object', 'x or y undefined, or their type are not object.');
        return x[name] > y[name] ? -1:1;
    };
    return fun;
}

function by(name, minor){
    let fun = function(x,y){
        assert(x && y && typeof x === 'object' && typeof y ==='object', 'x or y undefined, or their type are not object.');

        let a = x[name];
        let b = y[name];
        let comRet = int64Compare(b, a);

        if(comRet === 0){
            return typeof minor === 'function' ? minor(y, x) : 0;
        }
        else{
            return comRet;
        }

    };
    return fun;
}

function getObjectMetaData(key){
    assert(typeof key === 'string', 'Args type error, key must be a string.');

    let data = storageLoad(key);
    assert(data !== false, 'Get ' + key + ' from metadata failed.');

    let value = JSON.parse(data);
    return value;
}

function setMetaData(key, value)
{
    assert(typeof key === 'string', 'Args type error. key must be a string.');

    if(value === undefined){
        storageDel(key);
        log('Delete (' + key + ') from metadata succeed.');
    }
    else{
        let strVal = JSON.stringify(value);
        storageStore(key, strVal);
        log('Set key(' + key + '), value(' + strVal + ') in metadata succeed.');
    }
}

function transferCoin(dest, amount)
{
    assert((typeof dest === 'string') && (typeof amount === 'string'), 'Args type error. arg-dest and arg-amount must be a string.');
    if(amount === '0'){ return true; }

    payCoin(dest, amount);
    log('Pay coin( ' + amount + ') to dest account(' + dest + ') succeed.');
}

function findI0(arr, key){
    assert((typeof arr === 'object') && (typeof key === 'string'), 'Args type error. arg-arr must be an object, and arg-key must be a string.');

    let i = 0;
    while(i < arr.length){
        if(arr[i][0] === key){
            break;
        }
        i += 1;
    }

    if(i !== arr.length){
        return i;
    }
    else{
        return false;
    }
}

function insertCandidatesSorted(applicant, amount, candidates){
    assert(typeof applicant === 'string' && typeof amount === 'string' && typeof candidates === 'object', 'args error, arg-applicant and arg-amount must be string, arg-candidates must be arrary.');

    if(candidates.length >= (validatorSetSize * 2)){
        log('Validator candidates is enough.');
        return false;
    }

    if(candidates.length === 0){
        candidates.push([applicant, amount]);
        return candidates;
    }

    let i = 0;
    while(i < candidates.length){
        if(int64Compare(amount, candidates[i][1]) >= 0){ break; }
        i += 1;
    }

    if(i >= candidates.length){
        candidates.splice(i, 0, [applicant, amount]);
        return candidates;
    }

    if(amount === candidates[i][1]){
        while(i < candidates.length){
            if(applicant <= candidates[i][0] || int64Compare(amount, candidates[i][1]) > 0){ break; }
            i += 1;
        }
    }

    candidates.splice(i, 0, [applicant, amount]);
    return candidates;
}

function setValidatorsFromCandidate(candidates){
    let validators    = candidates.slice(0, validatorSetSize);
    let validatorsStr = JSON.stringify(validators);
    setValidators(validatorsStr);
    log('Set new validator sets(' + validatorsStr + ') succeed.');
    return true;
}

function applyAsValidatorCandidate(){
    let candidates = getObjectMetaData(candidatesVar);
    let position   = findI0(candidates, sender);

    if (position !== false){
        let comc = int64Compare(thisPayCoinAmount, minSuperadditionAmount);
        assert(comc === 1 || comc === 0, 'Superaddtion coin amount must more than ' + minSuperadditionAmount);

        let amountc = int64Add(candidates[position][1], thisPayCoinAmount);
        candidates.splice(position, 1);
        let newCandidates = insertCandidatesSorted(sender, amountc, candidates);
        setMetaData(candidatesVar, newCandidates);

        if(findI0(newCandidates, sender) < validatorSetSize){
            setValidatorsFromCandidate(newCandidates);
        }
    }
    else{
        let applicant = {};
        let applicantKey = applicantVar + sender;
        let applicantStr = storageLoad(applicantKey);
        if(applicantStr !== false){
            let coms = int64Compare(thisPayCoinAmount, minSuperadditionAmount);
            assert(coms === 1 || coms === 0, 'Superaddtion coin amount must more than ' + minSuperadditionAmount);

            applicant = JSON.parse(applicantStr); 
            let amountp = int64Add(applicant[pledgeAmountVar], thisPayCoinAmount);
            applicant[pledgeAmountVar] = amountp;
       }
       else{
            let comp = int64Compare(thisPayCoinAmount, minPledgeAmount);
            assert(comp === 1 || comp === 0, 'Pledge coin amount must more than ' + minPledgeAmount);
            applicant[pledgeAmountVar] = thisPayCoinAmount;
            applicant[ballotVar] = [];
       }

       /*Additional deposit allows you to update the deadline for voting*/
       applicant[expiredTimeVar] = blockTimestamp + effectiveVoteInterval;
       setMetaData(applicantKey, applicant);
   }

   return true;
}

function voteForApplicant(applicant){
    assert(addressCheck(applicant) === true, 'Arg-applicant is not valid adress.');

    let validators = getValidators();
    assert(validators !== false, 'Get validators failed.');
    assert(findI0(validators, sender) !== false,  sender + ' has no permission to vote.');

    let applicantKey = applicantVar + applicant;
    let applicantStr = storageLoad(applicantKey);
    if(applicantStr === false){
        log(applicantKey + ' is not existed, voting maybe passed or expired.');
        return false;
    }

    let applicantData = JSON.parse(applicantStr);
    if(blockTimestamp > applicantData[expiredTimeVar]){
        log('Vote time is expired, applicant ' + applicant + ' be refused.');
        transferCoin(applicant, applicantData[pledgeAmountVar]);
        setMetaData(applicantKey);
        return false;
    }

    let candidates = getObjectMetaData(candidatesVar);
    if(candidates.length >= (validatorSetSize * 2)){
        log('Validator candidates are enough');
        return false;
    }

    assert(applicantData[ballotVar].includes(sender) !== true, sender + ' has voted.');
    applicantData[ballotVar].push(sender);
    if(Object.keys(applicantData[ballotVar]).length < parseInt(validators.length * passRate + 0.5)){
        setMetaData(applicantKey, applicantData);
        return true;
    }

    let newCandidates = insertCandidatesSorted(applicant, applicantData[pledgeAmountVar], candidates);
    setMetaData(candidatesVar, newCandidates);
    setMetaData(applicantKey);

    if(findI0(newCandidates, applicant) < validatorSetSize){
        setValidatorsFromCandidate(newCandidates);
    }

    return true;
}

function takebackAllPledgeCoin(){
    let applicantKey = applicantVar + sender;
    let applicantStr = storageLoad(applicantKey);
    if(applicantStr !== false){
        let applicantData = JSON.parse(applicantStr);
        transferCoin(sender, applicantData[pledgeAmountVar]);
        setMetaData(applicantKey);
        return true;
    }

    let candidates = getObjectMetaData(candidatesVar);
    let position = findI0(candidates, sender);
    if(position !== false){
        assert(candidates.length > 1, 'The number of validators must > 1.');
        transferCoin(sender, candidates[position][1]);
        candidates.splice(position, 1);
        setMetaData(candidatesVar, candidates);

        if(position < validatorSetSize){
            setValidatorsFromCandidate(candidates);
        }
    }
    return true;
}

function abolishValidator(malicious, proof){
    assert(addressCheck(malicious) === true, 'Arg-malicious is not valid adress.');
    assert(typeof proof === 'string', 'Args type error, arg-proof must be string.'); 

    let validators = getValidators();
    assert(validators !== false, 'Get validators failed.');
    assert(findI0(validators, sender) !== false, sender + ' has no permmition to abolish validator.'); 
    assert(findI0(validators, malicious) !== false, 'current validator sets has no ' + malicious); 

    let abolishKey = abolishVar + malicious;
    let abolishStr = storageLoad(abolishKey);
    if(abolishStr !== false){
        let abolishProposal = JSON.parse(abolishStr);
        if(blockTimestamp >= abolishProposal[expiredTimeVar]){
            log('Update expired time of abolishing validator(' + malicious + ').'); 
            abolishProposal[expiredTimeVar] = blockTimestamp;
            setMetaData(abolishKey, abolishProposal);
        }
        else{
            log('Already abolished validator(' + malicious + ').'); 
        }

        return true;
    }

    let newProposal = {};
    newProposal[abolishVar]     = malicious;
    newProposal[reasonVar]      = proof;
    newProposal[proposerVar]    = sender;
    newProposal[expiredTimeVar] = blockTimestamp + effectiveVoteInterval;
    newProposal[ballotVar]      = [sender];

    setMetaData(abolishKey, newProposal);
    return true;
}

function quitAbolishValidator(malicious){
    assert(addressCheck(malicious) === true, 'Arg-malicious is not valid adress.');

    let abolishKey = abolishVar + malicious;
    let abolishProposal = getObjectMetaData(abolishKey);
    assert(sender === abolishProposal[proposerVar], sender + ' is not proposer, has no permission to quit the abolishProposal.');

    setMetaData(abolishKey);
    return true;
}

function voteAbolishValidator(malicious){
    assert(addressCheck(malicious) === true, 'Arg-malicious is not valid adress.');

    let validators = getValidators();
    assert(validators !== false, 'Get validators failed.');
    assert(validators.length > 1, 'The number of validators must > 1.');
    assert(findI0(validators, sender) !== false, sender + ' has no permission to vote.'); 
    assert(findI0(validators, malicious) !== false, malicious + ' is not validator.'); 

    let abolishKey = abolishVar + malicious;
    let abolishStr = storageLoad(abolishKey);
    if(abolishStr === false){
        log(abolishKey + ' is not existed, voting maybe passed or expired.');
        return false;
    }

    let abolishProposal = JSON.parse(abolishStr);
    if(blockTimestamp >abolishProposal[expiredTimeVar]){
        log('Voting time expired, ' + malicious + ' is still validator.'); 
        setMetaData(abolishKey);
        return false;
    }
    
    assert(abolishProposal[ballotVar].includes(sender) !== true, sender + ' has voted.');
    abolishProposal[ballotVar].push(sender);
    if(Object.keys(abolishProposal[ballotVar]).length < parseInt(validators.length * passRate + 0.5)){
        setMetaData(abolishKey, abolishProposal);
        return true;
    }

    let candidates = getObjectMetaData(candidatesVar);
    let position   = findI0(candidates, malicious); /*step here, logic promising position !== false*/
    let forfeit    = int64Div(candidates[position][1], 10);
    let leftCoin   = int64Sub(candidates[position][1], forfeit);

    transferCoin(malicious, leftCoin);
    setMetaData(abolishKey);
    candidates.splice(position, 1);

    let leftValidatorsCnt = validators.length - 1;
    let award   = int64Mod(forfeit, leftValidatorsCnt);
    let average = int64Div(forfeit, leftValidatorsCnt);
    if(award !== '0'){
        candidates[0][1] = int64Add(candidates[0][1], award);
    }

    let i = 0;
    while(i < leftValidatorsCnt){
        candidates[i][1] = int64Add(candidates[i][1], average);
        i += 1;
    }

    setMetaData(candidatesVar, candidates);
    setValidatorsFromCandidate(candidates);
    return true;
}

function query(input_str){
    let input  = JSON.parse(input_str);

    let result = {};
    if(input.method === 'getValidators'){
        result.current_validators = getValidators();
    }
    else if(input.method === 'getCandidates'){
        result.current_candidates = storageLoad(candidatesVar);
    }
    else if(input.method === 'getApplicantProposal'){
        result.application_proposal = storageLoad(applicantVar + input.params.address);
    }
    else if(input.method === 'getAbolishProposal'){
        result.abolish_proposal = storageLoad(abolishVar + input.params.address);
    }
    else{
       	throw '<unidentified operation type>';
    }

    log(result);
    return JSON.stringify(result);
}

function main(input_str){
    let input = JSON.parse(input_str);

    if(input.method === 'pledgeCoin'){
        applyAsValidatorCandidate();
    }
    else if(input.method === 'voteForApplicant'){
	    voteForApplicant(input.params.address);
    }
    else if(input.method === 'takebackCoin'){
	    takebackAllPledgeCoin();
    }
    else if(input.method === 'abolishValidator'){
    	abolishValidator(input.params.address, input.params.proof);
    }
    else if(input.method === 'quitAbolish'){
    	quitAbolishValidator(input.params.address);
    }
    else if(input.method === 'voteForAbolish'){
    	voteAbolishValidator(input.params.address);
    }
    else{
        throw '<undidentified operation type>';
    }
}

function init(){
    let validators = getValidators();
    assert(validators !== false, 'Get validators failed.');

    let initCandidates = validators.sort(by(1, byString(0)));
    let candidateStr   = JSON.stringify(initCandidates);
    storageStore(candidatesVar, candidateStr);
    setValidators(candidateStr);

    return true;
}
