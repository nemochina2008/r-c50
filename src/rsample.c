/*************************************************************************/
/*									 */
/*	 Source code for use with See5/C5.0 Release 2.08		 */
/*	 -----------------------------------------------		 */
/*		       Copyright RuleQuest Research 2011		 */
/*									 */
/*	This code is provided "as is" without warranty of any kind,	 */
/*	either express or implied.  All use is at your own risk.	 */
/*									 */
/*************************************************************************/


/*************************************************************************/
/*									 */
/*	Sample program to demonstrate the use of See5/C5.0 classifiers	 */
/*	--------------------------------------------------------------	 */
/*									 */
/*	Compilation:							 */
/*									 */
/*	    Unix: use an ANSI C compiler such as gcc and include	 */
/*		  the math library, e.g. gcc sample.c -lm		 */
/*									 */
/*	    Windows: compile as a console application with symbol	 */
/*		  "WIN32" defined					 */
/*									 */
/*	This program accepts three command-line options:		 */
/*									 */
/*	    -f <filestem>   specify the application name (required)	 */
/*	    -r		    use rulesets instead of decision trees	 */
/*	    -R		    use rulesets and show rules used		 */
/*	    -x		    use a similar format as saved by the	 */
/*			    See5 cross-reference window			 */
/*									 */
/*	The program expects to find the following files:		 */
/*									 */
/*	    <filestem>.names (the application names file)		 */
/*									 */
/*	    <filestem>.rules or <filestem>.tree (the classifier	files	 */
/*		 generated by C5.0 or See5)			 	 */
/*									 */
/*	    <filestem>.costs (optional -- the  costs file)		 */
/*									 */
/*	    <filestem>.cases (with a format similar to a .data file, but */
/*		allowing classes to be given as '?' meaning 'unknown')	 */
/*									 */
/*	Please note: the names file must be exactly as it was when	 */
/*	the classifier was generated.					 */
/*									 */
/*	For each case in <filestem.cases>, the program prints the	 */
/*	given class and then the class predicted by the classifier	 */
/*	together with the confidence of the prediction.			 */
/*									 */
/*	Revised January 2011						 */
/*									 */
/*************************************************************************/

#include "defns.h"
#include "extern.h"

extern void FreeGlobals();
extern void Rprintf(const char *, ...);
extern int SetTrials (int *internal ,int user);

/*************************************************************************/
/*									 */
/*	Main                                                             */
/*									 */
/*************************************************************************/

int rpredictmain (int *trials ,int *outputv ,double *confidencev)
/*  ----------  */
{
    FILE		*F;
    DataRec		Case;
    int			CaseNo=0, MaxClassLen=5, o, TotalRules=0,
    StartList, CurrentPosition, RealTrials;
    double               TotalConf=0, NumClasses=0;
    ClassNo		Predict, c;
//    Boolean		XRefForm=false;
//    void		ShowRules(int);
    int                 i;

    /* TRIALS is a global variable that is derived from the rule/tree
     file and is the value specified at the time of the model build. 
     The integer inside *trials is the value that was used when calling 
     predict.C5.0. That R code passes a value of zero if the default 
     value of trials is used */ 
    
    MODE = m_predict;

    /*  Read information on attribute names, values, and classes  */

    if ( ! (F = GetFile(".names", "r")) ) Error(NOFILE, Fn, "");

    GetNames(F);

    /*  Read the appropriate classifier file.  Call CheckFile() to
	determine the number of trials, then allocate space for
	trees or rulesets  */


    if ( RULES )
    {
	CheckFile(".rules", false);
	SetTrials(&TRIALS ,*trials);
	RuleSet = AllocZero(TRIALS+1, CRuleSet);
        // Rprintf("TRIALS: %4d\n", TRIALS);
	ForEach(Trial, 0, TRIALS-1)
	{
	    RuleSet[Trial] = GetRules(".rules");
	    TotalRules += RuleSet[Trial]->SNRules;
	}
 
	/*
	if ( RULESUSED )
	{
	    RulesUsed = Alloc(TotalRules + TRIALS, RuleNo);
	}
	*/

	MostSpec = Alloc(MaxClass+1, CRule);
    }
    else
    {
	CheckFile(".tree", false);
	SetTrials(&TRIALS ,*trials);
	Pruned = AllocZero(TRIALS+1, Tree);

	ForEach(Trial, 0, TRIALS-1)
	{
	    Pruned[Trial] = GetTree(".tree");
	}
    }

    /*  Set global default class for boosting  */

    Default = ( RULES ? RuleSet[0]->SDefault : Pruned[0]->Leaf );

    /*  Now classify the cases in file <filestem>.cases.
	This has the same format as a .data file except that
	the class can be "?" to indicate that it is unknown.  */

//    if ( XRefForm )
//    {
//	ForEach(c, 1, MaxClass)
//	{
//	    if ( (o = strlen(ClassName[c])) > MaxClassLen ) MaxClassLen = o;
//	}
//
//	printf("%-15s %*s   [Predicted]%s\n\n",
//	       "Case", -MaxClassLen, "Class",
//	       ( RULESUSED ? "   Rules" : "" ));
//
//	StartList = 16 + MaxClassLen + 3 +
//		    ( MaxClassLen > 9 ? MaxClassLen + 2 : 11 ) + 3;
//    }
//    else
//    {
//	printf("Case\t\tGiven\t\tPredicted%s\n %s\t\tClass\t\tClass\n\n",
//		( RULESUSED ? "\t\t    Rules" : "" ),
//		( LabelAtt ? "ID" : "No" ));

	StartList = 60;
//    }

    if ( ! (F = GetFile(".cases", "r")) ) Error(NOFILE, Fn, "");

    ClassSum = AllocZero(MaxClass+1, double);   /* used in classification */
    Vote     = AllocZero(MaxClass+1, float);   /* used with boosting */

    LineNo = 0;

    i = 0;  // XXX added this at least temporarily

    while ( (Case = PredictGetDataRec(F, false)) )
    {
	/*  For this case, find the class predicted by See5/C5.0 model  */

	Predict = PredictClassify(Case);

	/* XXX prediction is ClassName[Predict]? */
	outputv[i] = Predict;  // XXX add one?
        TotalConf = 0;
        NumClasses = 0;
	ForEach(c ,1 ,MaxClass) {
	    confidencev[MaxClass*i+c-1] = ClassSum[c] ;
            TotalConf += ClassSum[c];
            NumClasses += 1;
	}
        
        // AMK In case no rule is triggered
        if(TotalConf == 0){
            ForEach(c ,1 ,MaxClass) {
                confidencev[MaxClass*i+c-1] = 1/NumClasses;
            }
            TotalConf = 1;            
        }
        // AMK if a class has no active rules, normalize the conf values.
        // In other cases, the probabilities don't exactly add up (e.g. 0.999999920630845)
        ForEach(c ,1 ,MaxClass) {
            confidencev[MaxClass*i+c-1] = confidencev[MaxClass*i+c-1]/TotalConf;
        }            

	/*  Print either case label or number  */

//      if ( LabelAtt )
//      {
//          printf("%-15.15s ", (String) (IgnoredVals + SVal(Case,LabelAtt)));
//      }
//      else
//      {
//          printf("%4d\t\t", ++CaseNo);
//      }
//
	/*  Print the result for this case in alternative formats  */
//
//      if ( XRefForm )
//      {
//          printf("%*s", -MaxClassLen, ClassName[Class(Case)]);
//          CurrentPosition = 16 + MaxClassLen;
//
//          if ( Class(Case) != Predict )
//          {
// 		printf("   [%s]", ClassName[Predict]);
// 		CurrentPosition += 5 + strlen(ClassName[Predict]);
// 	    }
// 	}
// 	else
// 	{
// 	    printf("%-15.15s %-15.15s [%.2f]",
// 		    ClassName[Class(Case)], ClassName[Predict], Confidence);
// 	    CurrentPosition = 54;
// 	}

// 	if ( RULESUSED ) ShowRules(StartList - CurrentPosition);

// 	printf("\n");

	/*  Free the memory used by this case  */

	PredictFreeLastCase(Case);

	i++;
    }

    /*  Close the case file and free allocated memory  */
    
    // XXX This causes a segfault, but need to verify that
    // XXX removing this doesn't cause a memory leak
    // FreeGlobals();

    free(ClassSum);
    free(Vote);

    FreeNames();

    ForEach(Trial, 0, TRIALS-1) {
        FreeTree(Pruned[Trial]);
    }
    free(Pruned);
    
    return 0;
}



/*************************************************************************/
/*									 */
/*	Show rules that were used to classify a case.			 */
/*	Classify() will have set RulesUsed[] to				 */
/*	  number of active rules for trial 0,				 */
/*	  first active rule, second active rule, ..., last active rule,	 */
/*	  number of active rules for trial 1,				 */
/*	  first active rule, second active rule, ..., last active rule,	 */
/*	and so on.							 */
/*									 */
/*************************************************************************/


void ShowRules(int Spaces)
/*   ---------  */
{
    int	p, pLast, a, b, First;

    Rprintf("%*s", Spaces, "");

    p = 0;
    ForEach(Trial, 0, TRIALS-1)
    {
	pLast = p + RulesUsed[p];

	ForEach(a, 1, RulesUsed[p])
	{
	    /*  Rules used are not in order, so find first  */

	    First = 0;

	    ForEach(b, p+1, pLast)
	    {
		if ( RulesUsed[b] &&
		     ( ! First || RulesUsed[b] < RulesUsed[First] ) )
		{
		    First = b;
		}
	    }

	    if ( TRIALS > 1 ) Rprintf("%d/", Trial);

	    Rprintf("%d ", RulesUsed[First]);

	    RulesUsed[First] = 0;
	}

	p = pLast + 1;
    }
}

int SetTrials (int *internal ,int user) {
    /*effects:
	    internal may be modified
    */

    /* If the user allowed early stopping of boosting, the internal 
     value for TRIALS may be less than the one specificed by the
     the user at prediction time. If this is the case, we reset
     TRIALS to the internal value, which is the largest value 
     possible. */

    // Rprintf("internal TRIALS value = %d\n user trials value = %d\n", *internal, user);
    if (user > 0 && user <= *internal) {
	*internal = user;
	// Rprintf("using user-specified trials value");
    } else {
	// Rprintf("using internal trials value");
    }
}
