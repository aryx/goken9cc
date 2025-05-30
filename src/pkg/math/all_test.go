// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package math_test

import (
	"fmt"
	. "math"
	"runtime"
	"testing"
)

var vf = []float64{
	4.9790119248836735e+00,
	7.7388724745781045e+00,
	-2.7688005719200159e-01,
	-5.0106036182710749e+00,
	9.6362937071984173e+00,
	2.9263772392439646e+00,
	5.2290834314593066e+00,
	2.7279399104360102e+00,
	1.8253080916808550e+00,
	-8.6859247685756013e+00,
}
// The expected results below were computed by the high precision calculators
// at http://keisan.casio.com/.  More exact input values (array vf[], above)
// were obtained by printing them with "%.26f".  The answers were calculated
// to 26 digits (by using the "Digit number" drop-down control of each
// calculator).
var acos = []float64{
	1.0496193546107222142571536e+00,
	6.8584012813664425171660692e-01,
	1.5984878714577160325521819e+00,
	2.0956199361475859327461799e+00,
	2.7053008467824138592616927e-01,
	1.2738121680361776018155625e+00,
	1.0205369421140629186287407e+00,
	1.2945003481781246062157835e+00,
	1.3872364345374451433846657e+00,
	2.6231510803970463967294145e+00,
}
var acosh = []float64{
	2.4743347004159012494457618e+00,
	2.8576385344292769649802701e+00,
	7.2796961502981066190593175e-01,
	2.4796794418831451156471977e+00,
	3.0552020742306061857212962e+00,
	2.044238592688586588942468e+00,
	2.5158701513104513595766636e+00,
	1.99050839282411638174299e+00,
	1.6988625798424034227205445e+00,
	2.9611454842470387925531875e+00,
}
var asin = []float64{
	5.2117697218417440497416805e-01,
	8.8495619865825236751471477e-01,
	-02.769154466281941332086016e-02,
	-5.2482360935268931351485822e-01,
	1.3002662421166552333051524e+00,
	2.9698415875871901741575922e-01,
	5.5025938468083370060258102e-01,
	2.7629597861677201301553823e-01,
	1.83559892257451475846656e-01,
	-1.0523547536021497774980928e+00,
}
var asinh = []float64{
	2.3083139124923523427628243e+00,
	2.743551594301593620039021e+00,
	-2.7345908534880091229413487e-01,
	-2.3145157644718338650499085e+00,
	2.9613652154015058521951083e+00,
	1.7949041616585821933067568e+00,
	2.3564032905983506405561554e+00,
	1.7287118790768438878045346e+00,
	1.3626658083714826013073193e+00,
	-2.8581483626513914445234004e+00,
}
var atan = []float64{
	1.372590262129621651920085e+00,
	1.442290609645298083020664e+00,
	-2.7011324359471758245192595e-01,
	-1.3738077684543379452781531e+00,
	1.4673921193587666049154681e+00,
	1.2415173565870168649117764e+00,
	1.3818396865615168979966498e+00,
	1.2194305844639670701091426e+00,
	1.0696031952318783760193244e+00,
	-1.4561721938838084990898679e+00,
}
var atanh = []float64{
	5.4651163712251938116878204e-01,
	1.0299474112843111224914709e+00,
	-2.7695084420740135145234906e-02,
	-5.5072096119207195480202529e-01,
	1.9943940993171843235906642e+00,
	3.01448604578089708203017e-01,
	5.8033427206942188834370595e-01,
	2.7987997499441511013958297e-01,
	1.8459947964298794318714228e-01,
	-1.3273186910532645867272502e+00,
}
var atan2 = []float64{
	1.1088291730037004444527075e+00,
	9.1218183188715804018797795e-01,
	1.5984772603216203736068915e+00,
	2.0352918654092086637227327e+00,
	8.0391819139044720267356014e-01,
	1.2861075249894661588866752e+00,
	1.0889904479131695712182587e+00,
	1.3044821793397925293797357e+00,
	1.3902530903455392306872261e+00,
	2.2859857424479142655411058e+00,
}
var cbrt = []float64{
	1.7075799841925094446722675e+00,
	1.9779982212970353936691498e+00,
	-6.5177429017779910853339447e-01,
	-1.7111838886544019873338113e+00,
	2.1279920909827937423960472e+00,
	1.4303536770460741452312367e+00,
	1.7357021059106154902341052e+00,
	1.3972633462554328350552916e+00,
	1.2221149580905388454977636e+00,
	-2.0556003730500069110343596e+00,
}
var ceil = []float64{
	5.0000000000000000e+00,
	8.0000000000000000e+00,
	0.0000000000000000e+00,
	-5.0000000000000000e+00,
	1.0000000000000000e+01,
	3.0000000000000000e+00,
	6.0000000000000000e+00,
	3.0000000000000000e+00,
	2.0000000000000000e+00,
	-8.0000000000000000e+00,
}
var copysign = []float64{
	-4.9790119248836735e+00,
	-7.7388724745781045e+00,
	-2.7688005719200159e-01,
	-5.0106036182710749e+00,
	-9.6362937071984173e+00,
	-2.9263772392439646e+00,
	-5.2290834314593066e+00,
	-2.7279399104360102e+00,
	-1.8253080916808550e+00,
	-8.6859247685756013e+00,
}
var cos = []float64{
	2.634752140995199110787593e-01,
	1.148551260848219865642039e-01,
	9.6191297325640768154550453e-01,
	2.938141150061714816890637e-01,
	-9.777138189897924126294461e-01,
	-9.7693041344303219127199518e-01,
	4.940088096948647263961162e-01,
	-9.1565869021018925545016502e-01,
	-2.517729313893103197176091e-01,
	-7.39241351595676573201918e-01,
}
var cosh = []float64{
	7.2668796942212842775517446e+01,
	1.1479413465659254502011135e+03,
	1.0385767908766418550935495e+00,
	7.5000957789658051428857788e+01,
	7.655246669605357888468613e+03,
	9.3567491758321272072888257e+00,
	9.331351599270605471131735e+01,
	7.6833430994624643209296404e+00,
	3.1829371625150718153881164e+00,
	2.9595059261916188501640911e+03,
}
var erf = []float64{
	5.1865354817738701906913566e-01,
	7.2623875834137295116929844e-01,
	-3.123458688281309990629839e-02,
	-5.2143121110253302920437013e-01,
	8.2704742671312902508629582e-01,
	3.2101767558376376743993945e-01,
	5.403990312223245516066252e-01,
	3.0034702916738588551174831e-01,
	2.0369924417882241241559589e-01,
	-7.8069386968009226729944677e-01,
}
var erfc = []float64{
	4.8134645182261298093086434e-01,
	2.7376124165862704883070156e-01,
	1.0312345868828130999062984e+00,
	1.5214312111025330292043701e+00,
	1.7295257328687097491370418e-01,
	6.7898232441623623256006055e-01,
	4.596009687776754483933748e-01,
	6.9965297083261411448825169e-01,
	7.9630075582117758758440411e-01,
	1.7806938696800922672994468e+00,
}
var exp = []float64{
	1.4533071302642137507696589e+02,
	2.2958822575694449002537581e+03,
	7.5814542574851666582042306e-01,
	6.6668778421791005061482264e-03,
	1.5310493273896033740861206e+04,
	1.8659907517999328638667732e+01,
	1.8662167355098714543942057e+02,
	1.5301332413189378961665788e+01,
	6.2047063430646876349125085e+00,
	1.6894712385826521111610438e-04,
}
var expm1 = []float64{
	5.105047796122957327384770212e-02,
	8.046199708567344080562675439e-02,
	-2.764970978891639815187418703e-03,
	-4.8871434888875355394330300273e-02,
	1.0115864277221467777117227494e-01,
	2.969616407795910726014621657e-02,
	5.368214487944892300914037972e-02,
	2.765488851131274068067445335e-02,
	1.842068661871398836913874273e-02,
	-8.3193870863553801814961137573e-02,
}
var exp2 = []float64{
	3.1537839463286288034313104e+01,
	2.1361549283756232296144849e+02,
	8.2537402562185562902577219e-01,
	3.1021158628740294833424229e-02,
	7.9581744110252191462569661e+02,
	7.6019905892596359262696423e+00,
	3.7506882048388096973183084e+01,
	6.6250893439173561733216375e+00,
	3.5438267900243941544605339e+00,
	2.4281533133513300984289196e-03,
}
var fabs = []float64{
	4.9790119248836735e+00,
	7.7388724745781045e+00,
	2.7688005719200159e-01,
	5.0106036182710749e+00,
	9.6362937071984173e+00,
	2.9263772392439646e+00,
	5.2290834314593066e+00,
	2.7279399104360102e+00,
	1.8253080916808550e+00,
	8.6859247685756013e+00,
}
var fdim = []float64{
	4.9790119248836735e+00,
	7.7388724745781045e+00,
	0.0000000000000000e+00,
	0.0000000000000000e+00,
	9.6362937071984173e+00,
	2.9263772392439646e+00,
	5.2290834314593066e+00,
	2.7279399104360102e+00,
	1.8253080916808550e+00,
	0.0000000000000000e+00,
}
var floor = []float64{
	4.0000000000000000e+00,
	7.0000000000000000e+00,
	-1.0000000000000000e+00,
	-6.0000000000000000e+00,
	9.0000000000000000e+00,
	2.0000000000000000e+00,
	5.0000000000000000e+00,
	2.0000000000000000e+00,
	1.0000000000000000e+00,
	-9.0000000000000000e+00,
}
var fmod = []float64{
	4.197615023265299782906368e-02,
	2.261127525421895434476482e+00,
	3.231794108794261433104108e-02,
	4.989396381728925078391512e+00,
	3.637062928015826201999516e-01,
	1.220868282268106064236690e+00,
	4.770916568540693347699744e+00,
	1.816180268691969246219742e+00,
	8.734595415957246977711748e-01,
	1.314075231424398637614104e+00,
}

type fi struct {
	f float64
	i int
}

var frexp = []fi{
	fi{6.2237649061045918750e-01, 3},
	fi{9.6735905932226306250e-01, 3},
	fi{-5.5376011438400318000e-01, -1},
	fi{-6.2632545228388436250e-01, 3},
	fi{6.02268356699901081250e-01, 4},
	fi{7.3159430981099115000e-01, 2},
	fi{6.5363542893241332500e-01, 3},
	fi{6.8198497760900255000e-01, 2},
	fi{9.1265404584042750000e-01, 1},
	fi{-5.4287029803597508250e-01, 4},
}
var gamma = []float64{
	2.3254348370739963835386613898e+01,
	2.991153837155317076427529816e+03,
	-4.561154336726758060575129109e+00,
	7.719403468842639065959210984e-01,
	1.6111876618855418534325755566e+05,
	1.8706575145216421164173224946e+00,
	3.4082787447257502836734201635e+01,
	1.579733951448952054898583387e+00,
	9.3834586598354592860187267089e-01,
	-2.093995902923148389186189429e-05,
}
var j0 = []float64{
	-1.8444682230601672018219338e-01,
	2.27353668906331975435892e-01,
	9.809259936157051116270273e-01,
	-1.741170131426226587841181e-01,
	-2.1389448451144143352039069e-01,
	-2.340905848928038763337414e-01,
	-1.0029099691890912094586326e-01,
	-1.5466726714884328135358907e-01,
	3.252650187653420388714693e-01,
	-8.72218484409407250005360235e-03,
}
var j1 = []float64{
	-3.251526395295203422162967e-01,
	1.893581711430515718062564e-01,
	-1.3711761352467242914491514e-01,
	3.287486536269617297529617e-01,
	1.3133899188830978473849215e-01,
	3.660243417832986825301766e-01,
	-3.4436769271848174665420672e-01,
	4.329481396640773768835036e-01,
	5.8181350531954794639333955e-01,
	-2.7030574577733036112996607e-01,
}
var j2 = []float64{
	5.3837518920137802565192769e-02,
	-1.7841678003393207281244667e-01,
	9.521746934916464142495821e-03,
	4.28958355470987397983072e-02,
	2.4115371837854494725492872e-01,
	4.842458532394520316844449e-01,
	-3.142145220618633390125946e-02,
	4.720849184745124761189957e-01,
	3.122312022520957042957497e-01,
	7.096213118930231185707277e-02,
}
var jM3 = []float64{
	-3.684042080996403091021151e-01,
	2.8157665936340887268092661e-01,
	4.401005480841948348343589e-04,
	3.629926999056814081597135e-01,
	3.123672198825455192489266e-02,
	-2.958805510589623607540455e-01,
	-3.2033177696533233403289416e-01,
	-2.592737332129663376736604e-01,
	-1.0241334641061485092351251e-01,
	-2.3762660886100206491674503e-01,
}
var lgamma = []fi{
	fi{3.146492141244545774319734e+00, 1},
	fi{8.003414490659126375852113e+00, 1},
	fi{1.517575735509779707488106e+00, -1},
	fi{-2.588480028182145853558748e-01, 1},
	fi{1.1989897050205555002007985e+01, 1},
	fi{6.262899811091257519386906e-01, 1},
	fi{3.5287924899091566764846037e+00, 1},
	fi{4.5725644770161182299423372e-01, 1},
	fi{-6.363667087767961257654854e-02, 1},
	fi{-1.077385130910300066425564e+01, -1},
}
var log = []float64{
	1.605231462693062999102599e+00,
	2.0462560018708770653153909e+00,
	-1.2841708730962657801275038e+00,
	1.6115563905281545116286206e+00,
	2.2655365644872016636317461e+00,
	1.0737652208918379856272735e+00,
	1.6542360106073546632707956e+00,
	1.0035467127723465801264487e+00,
	6.0174879014578057187016475e-01,
	2.161703872847352815363655e+00,
}
var logb = []float64{
	2.0000000000000000e+00,
	2.0000000000000000e+00,
	-2.0000000000000000e+00,
	2.0000000000000000e+00,
	3.0000000000000000e+00,
	1.0000000000000000e+00,
	2.0000000000000000e+00,
	1.0000000000000000e+00,
	0.0000000000000000e+00,
	3.0000000000000000e+00,
}
var log10 = []float64{
	6.9714316642508290997617083e-01,
	8.886776901739320576279124e-01,
	-5.5770832400658929815908236e-01,
	6.998900476822994346229723e-01,
	9.8391002850684232013281033e-01,
	4.6633031029295153334285302e-01,
	7.1842557117242328821552533e-01,
	4.3583479968917773161304553e-01,
	2.6133617905227038228626834e-01,
	9.3881606348649405716214241e-01,
}
var log1p = []float64{
	4.8590257759797794104158205e-02,
	7.4540265965225865330849141e-02,
	-2.7726407903942672823234024e-03,
	-5.1404917651627649094953380e-02,
	9.1998280672258624681335010e-02,
	2.8843762576593352865894824e-02,
	5.0969534581863707268992645e-02,
	2.6913947602193238458458594e-02,
	1.8088493239630770262045333e-02,
	-9.0865245631588989681559268e-02,
}
var log2 = []float64{
	2.3158594707062190618898251e+00,
	2.9521233862883917703341018e+00,
	-1.8526669502700329984917062e+00,
	2.3249844127278861543568029e+00,
	3.268478366538305087466309e+00,
	1.5491157592596970278166492e+00,
	2.3865580889631732407886495e+00,
	1.447811865817085365540347e+00,
	8.6813999540425116282815557e-01,
	3.118679457227342224364709e+00,
}
var modf = [][2]float64{
	[2]float64{4.0000000000000000e+00, 9.7901192488367350108546816e-01},
	[2]float64{7.0000000000000000e+00, 7.3887247457810456552351752e-01},
	[2]float64{0.0000000000000000e+00, -2.7688005719200159404635997e-01},
	[2]float64{-5.0000000000000000e+00, -1.060361827107492160848778e-02},
	[2]float64{9.0000000000000000e+00, 6.3629370719841737980004837e-01},
	[2]float64{2.0000000000000000e+00, 9.2637723924396464525443662e-01},
	[2]float64{5.0000000000000000e+00, 2.2908343145930665230025625e-01},
	[2]float64{2.0000000000000000e+00, 7.2793991043601025126008608e-01},
	[2]float64{1.0000000000000000e+00, 8.2530809168085506044576505e-01},
	[2]float64{-8.0000000000000000e+00, -6.8592476857560136238589621e-01},
}
var nextafter = []float64{
	4.97901192488367438926388786e+00,
	7.73887247457810545370193722e+00,
	-2.7688005719200153853520874e-01,
	-5.01060361827107403343006808e+00,
	9.63629370719841915615688777e+00,
	2.92637723924396508934364647e+00,
	5.22908343145930754047867595e+00,
	2.72793991043601069534929593e+00,
	1.82530809168085528249036997e+00,
	-8.68592476857559958602905681e+00,
}
var pow = []float64{
	9.5282232631648411840742957e+04,
	5.4811599352999901232411871e+07,
	5.2859121715894396531132279e-01,
	9.7587991957286474464259698e-06,
	4.328064329346044846740467e+09,
	8.4406761805034547437659092e+02,
	1.6946633276191194947742146e+05,
	5.3449040147551939075312879e+02,
	6.688182138451414936380374e+01,
	2.0609869004248742886827439e-09,
}
var remainder = []float64{
	4.197615023265299782906368e-02,
	2.261127525421895434476482e+00,
	3.231794108794261433104108e-02,
	-2.120723654214984321697556e-02,
	3.637062928015826201999516e-01,
	1.220868282268106064236690e+00,
	-4.581668629186133046005125e-01,
	-9.117596417440410050403443e-01,
	8.734595415957246977711748e-01,
	1.314075231424398637614104e+00,
}
var signbit = []bool{
	false,
	false,
	true,
	true,
	false,
	false,
	false,
	false,
	false,
	true,
}
var sin = []float64{
	-9.6466616586009283766724726e-01,
	9.9338225271646545763467022e-01,
	-2.7335587039794393342449301e-01,
	9.5586257685042792878173752e-01,
	-2.099421066779969164496634e-01,
	2.135578780799860532750616e-01,
	-8.694568971167362743327708e-01,
	4.019566681155577786649878e-01,
	9.6778633541687993721617774e-01,
	-6.734405869050344734943028e-01,
}
var sinh = []float64{
	7.2661916084208532301448439e+01,
	1.1479409110035194500526446e+03,
	-2.8043136512812518927312641e-01,
	-7.499429091181587232835164e+01,
	7.6552466042906758523925934e+03,
	9.3031583421672014313789064e+00,
	9.330815755828109072810322e+01,
	7.6179893137269146407361477e+00,
	3.021769180549615819524392e+00,
	-2.95950575724449499189888e+03,
}
var sqrt = []float64{
	2.2313699659365484748756904e+00,
	2.7818829009464263511285458e+00,
	5.2619393496314796848143251e-01,
	2.2384377628763938724244104e+00,
	3.1042380236055381099288487e+00,
	1.7106657298385224403917771e+00,
	2.286718922705479046148059e+00,
	1.6516476350711159636222979e+00,
	1.3510396336454586262419247e+00,
	2.9471892997524949215723329e+00,
}
var tan = []float64{
	-3.661316565040227801781974e+00,
	8.64900232648597589369854e+00,
	-2.8417941955033612725238097e-01,
	3.253290185974728640827156e+00,
	2.147275640380293804770778e-01,
	-2.18600910711067004921551e-01,
	-1.760002817872367935518928e+00,
	-4.389808914752818126249079e-01,
	-3.843885560201130679995041e+00,
	9.10988793377685105753416e-01,
}
var tanh = []float64{
	9.9990531206936338549262119e-01,
	9.9999962057085294197613294e-01,
	-2.7001505097318677233756845e-01,
	-9.9991110943061718603541401e-01,
	9.9999999146798465745022007e-01,
	9.9427249436125236705001048e-01,
	9.9994257600983138572705076e-01,
	9.9149409509772875982054701e-01,
	9.4936501296239685514466577e-01,
	-9.9999994291374030946055701e-01,
}
var trunc = []float64{
	4.0000000000000000e+00,
	7.0000000000000000e+00,
	-0.0000000000000000e+00,
	-5.0000000000000000e+00,
	9.0000000000000000e+00,
	2.0000000000000000e+00,
	5.0000000000000000e+00,
	2.0000000000000000e+00,
	1.0000000000000000e+00,
	-8.0000000000000000e+00,
}
var y0 = []float64{
	-3.053399153780788357534855e-01,
	1.7437227649515231515503649e-01,
	-8.6221781263678836910392572e-01,
	-3.100664880987498407872839e-01,
	1.422200649300982280645377e-01,
	4.000004067997901144239363e-01,
	-3.3340749753099352392332536e-01,
	4.5399790746668954555205502e-01,
	4.8290004112497761007536522e-01,
	2.7036697826604756229601611e-01,
}
var y1 = []float64{
	0.15494213737457922210218611,
	-0.2165955142081145245075746,
	-2.4644949631241895201032829,
	0.1442740489541836405154505,
	0.2215379960518984777080163,
	0.3038800915160754150565448,
	0.0691107642452362383808547,
	0.2380116417809914424860165,
	-0.20849492979459761009678934,
	0.0242503179793232308250804,
}
var y2 = []float64{
	0.3675780219390303613394936,
	-0.23034826393250119879267257,
	-16.939677983817727205631397,
	0.367653980523052152867791,
	-0.0962401471767804440353136,
	-0.1923169356184851105200523,
	0.35984072054267882391843766,
	-0.2794987252299739821654982,
	-0.7113490692587462579757954,
	-0.2647831587821263302087457,
}
var yM3 = []float64{
	-0.14035984421094849100895341,
	-0.097535139617792072703973,
	242.25775994555580176377379,
	-0.1492267014802818619511046,
	0.26148702629155918694500469,
	0.56675383593895176530394248,
	-0.206150264009006981070575,
	0.64784284687568332737963658,
	1.3503631555901938037008443,
	0.1461869756579956803341844,
}

// arguments and expected results for special cases
var vfacosSC = []float64{
	-Pi,
	1,
	Pi,
	NaN(),
}
var acosSC = []float64{
	NaN(),
	0,
	NaN(),
	NaN(),
}

var vfacoshSC = []float64{
	Inf(-1),
	0.5,
	1,
	Inf(1),
	NaN(),
}
var acoshSC = []float64{
	NaN(),
	NaN(),
	0,
	Inf(1),
	NaN(),
}

var vfasinSC = []float64{
	-Pi,
	Copysign(0, -1),
	0,
	Pi,
	NaN(),
}
var asinSC = []float64{
	NaN(),
	Copysign(0, -1),
	0,
	NaN(),
	NaN(),
}

var vfasinhSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var asinhSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}

var vfatanSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var atanSC = []float64{
	-Pi / 2,
	Copysign(0, -1),
	0,
	Pi / 2,
	NaN(),
}

var vfatanhSC = []float64{
	Inf(-1),
	-Pi,
	-1,
	Copysign(0, -1),
	0,
	1,
	Pi,
	Inf(1),
	NaN(),
}
var atanhSC = []float64{
	NaN(),
	NaN(),
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
	NaN(),
	NaN(),
}
var vfatan2SC = [][2]float64{
	[2]float64{Inf(-1), Inf(-1)},
	[2]float64{Inf(-1), -Pi},
	[2]float64{Inf(-1), 0},
	[2]float64{Inf(-1), +Pi},
	[2]float64{Inf(-1), Inf(1)},
	[2]float64{Inf(-1), NaN()},
	[2]float64{-Pi, Inf(-1)},
	[2]float64{-Pi, 0},
	[2]float64{-Pi, Inf(1)},
	[2]float64{-Pi, NaN()},
	[2]float64{Copysign(0, -1), Inf(-1)},
	[2]float64{Copysign(0, -1), -Pi},
	[2]float64{Copysign(0, -1), Copysign(0, -1)},
	[2]float64{Copysign(0, -1), 0},
	[2]float64{Copysign(0, -1), +Pi},
	[2]float64{Copysign(0, -1), Inf(1)},
	[2]float64{Copysign(0, -1), NaN()},
	[2]float64{0, Inf(-1)},
	[2]float64{0, -Pi},
	[2]float64{0, Copysign(0, -1)},
	[2]float64{0, 0},
	[2]float64{0, +Pi},
	[2]float64{0, Inf(1)},
	[2]float64{0, NaN()},
	[2]float64{+Pi, Inf(-1)},
	[2]float64{+Pi, 0},
	[2]float64{+Pi, Inf(1)},
	[2]float64{+Pi, NaN()},
	[2]float64{Inf(1), Inf(-1)},
	[2]float64{Inf(1), -Pi},
	[2]float64{Inf(1), 0},
	[2]float64{Inf(1), +Pi},
	[2]float64{Inf(1), Inf(1)},
	[2]float64{Inf(1), NaN()},
	[2]float64{NaN(), NaN()},
}
var atan2SC = []float64{
	-3 * Pi / 4,     // atan2(-Inf, -Inf)
	-Pi / 2,         // atan2(-Inf, -Pi)
	-Pi / 2,         // atan2(-Inf, +0)
	-Pi / 2,         // atan2(-Inf, +Pi)
	-Pi / 4,         // atan2(-Inf, +Inf)
	NaN(),           // atan2(-Inf, NaN)
	-Pi,             // atan2(-Pi, -Inf)
	-Pi / 2,         // atan2(-Pi, +0)
	Copysign(0, -1), // atan2(-Pi, Inf)
	NaN(),           // atan2(-Pi, NaN)
	-Pi,             // atan2(-0, -Inf)
	-Pi,             // atan2(-0, -Pi)
	-Pi,             // atan2(-0, -0)
	Copysign(0, -1), // atan2(-0, +0)
	Copysign(0, -1), // atan2(-0, +Pi)
	Copysign(0, -1), // atan2(-0, +Inf)
	NaN(),           // atan2(-0, NaN)
	Pi,              // atan2(+0, -Inf)
	Pi,              // atan2(+0, -Pi)
	Pi,              // atan2(+0, -0)
	0,               // atan2(+0, +0)
	0,               // atan2(+0, +Pi)
	0,               // atan2(+0, +Inf)
	NaN(),           // atan2(+0, NaN)
	Pi,              // atan2(+Pi, -Inf)
	Pi / 2,          // atan2(+Pi, +0)
	0,               // atan2(+Pi, +Inf)
	NaN(),           // atan2(+Pi, NaN)
	3 * Pi / 4,      // atan2(+Inf, -Inf)
	Pi / 2,          // atan2(+Inf, -Pi)
	Pi / 2,          // atan2(+Inf, +0)
	Pi / 2,          // atan2(+Inf, +Pi)
	Pi / 4,          // atan2(+Inf, +Inf)
	NaN(),           // atan2(+Inf, NaN)
	NaN(),           // atan2(NaN, NaN)
}

var vfcbrtSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var cbrtSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}

var vfceilSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var ceilSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}

var vfcopysignSC = []float64{
	Inf(-1),
	Inf(1),
	NaN(),
}
var copysignSC = []float64{
	Inf(-1),
	Inf(-1),
	NaN(),
}

var vfcosSC = []float64{
	Inf(-1),
	Inf(1),
	NaN(),
}
var cosSC = []float64{
	NaN(),
	NaN(),
	NaN(),
}

var vfcoshSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var coshSC = []float64{
	Inf(1),
	1,
	1,
	Inf(1),
	NaN(),
}

var vferfSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var erfSC = []float64{
	-1,
	Copysign(0, -1),
	0,
	1,
	NaN(),
}

var vferfcSC = []float64{
	Inf(-1),
	Inf(1),
	NaN(),
}
var erfcSC = []float64{
	2,
	0,
	NaN(),
}

var vfexpSC = []float64{
	Inf(-1),
	-2000,
	2000,
	Inf(1),
	NaN(),
}
var expSC = []float64{
	0,
	0,
	Inf(1),
	Inf(1),
	NaN(),
}

var vfexpm1SC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var expm1SC = []float64{
	-1,
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}

var vffabsSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var fabsSC = []float64{
	Inf(1),
	0,
	0,
	Inf(1),
	NaN(),
}

var vffmodSC = [][2]float64{
	[2]float64{Inf(-1), Inf(-1)},
	[2]float64{Inf(-1), -Pi},
	[2]float64{Inf(-1), 0},
	[2]float64{Inf(-1), Pi},
	[2]float64{Inf(-1), Inf(1)},
	[2]float64{Inf(-1), NaN()},
	[2]float64{-Pi, Inf(-1)},
	[2]float64{-Pi, 0},
	[2]float64{-Pi, Inf(1)},
	[2]float64{-Pi, NaN()},
	[2]float64{Copysign(0, -1), Inf(-1)},
	[2]float64{Copysign(0, -1), 0},
	[2]float64{Copysign(0, -1), Inf(1)},
	[2]float64{Copysign(0, -1), NaN()},
	[2]float64{0, Inf(-1)},
	[2]float64{0, 0},
	[2]float64{0, Inf(1)},
	[2]float64{0, NaN()},
	[2]float64{Pi, Inf(-1)},
	[2]float64{Pi, 0},
	[2]float64{Pi, Inf(1)},
	[2]float64{Pi, NaN()},
	[2]float64{Inf(1), Inf(-1)},
	[2]float64{Inf(1), -Pi},
	[2]float64{Inf(1), 0},
	[2]float64{Inf(1), Pi},
	[2]float64{Inf(1), Inf(1)},
	[2]float64{Inf(1), NaN()},
	[2]float64{NaN(), Inf(-1)},
	[2]float64{NaN(), -Pi},
	[2]float64{NaN(), 0},
	[2]float64{NaN(), Pi},
	[2]float64{NaN(), Inf(1)},
	[2]float64{NaN(), NaN()},
}
var fmodSC = []float64{
	NaN(),           // fmod(-Inf, -Inf)
	NaN(),           // fmod(-Inf, -Pi)
	NaN(),           // fmod(-Inf, 0)
	NaN(),           // fmod(-Inf, Pi)
	NaN(),           // fmod(-Inf, +Inf)
	NaN(),           // fmod(-Inf, NaN)
	-Pi,             // fmod(-Pi, -Inf)
	NaN(),           // fmod(-Pi, 0)
	-Pi,             // fmod(-Pi, +Inf)
	NaN(),           // fmod(-Pi, NaN)
	Copysign(0, -1), // fmod(-0, -Inf)
	NaN(),           // fmod(-0, 0)
	Copysign(0, -1), // fmod(-0, Inf)
	NaN(),           // fmod(-0, NaN)
	0,               // fmod(0, -Inf)
	NaN(),           // fmod(0, 0)
	0,               // fmod(0, +Inf)
	NaN(),           // fmod(0, NaN)
	Pi,              // fmod(Pi, -Inf)
	NaN(),           // fmod(Pi, 0)
	Pi,              // fmod(Pi, +Inf)
	NaN(),           // fmod(Pi, NaN)
	NaN(),           // fmod(+Inf, -Inf)
	NaN(),           // fmod(+Inf, -Pi)
	NaN(),           // fmod(+Inf, 0)
	NaN(),           // fmod(+Inf, Pi)
	NaN(),           // fmod(+Inf, +Inf)
	NaN(),           // fmod(+Inf, NaN)
	NaN(),           // fmod(NaN, -Inf)
	NaN(),           // fmod(NaN, -Pi)
	NaN(),           // fmod(NaN, 0)
	NaN(),           // fmod(NaN, Pi)
	NaN(),           // fmod(NaN, +Inf)
	NaN(),           // fmod(NaN, NaN)
}

var vffrexpSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var frexpSC = []fi{
	fi{Inf(-1), 0},
	fi{Copysign(0, -1), 0},
	fi{0, 0},
	fi{Inf(1), 0},
	fi{NaN(), 0},
}

var vfgammaSC = []float64{
	Inf(-1),
	-3,
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var gammaSC = []float64{
	Inf(-1),
	Inf(1),
	Inf(1),
	Inf(1),
	Inf(1),
	NaN(),
}

var vfhypotSC = [][2]float64{
	[2]float64{Inf(-1), Inf(-1)},
	[2]float64{Inf(-1), 0},
	[2]float64{Inf(-1), Inf(1)},
	[2]float64{Inf(-1), NaN()},
	[2]float64{Copysign(0, -1), Copysign(0, -1)},
	[2]float64{Copysign(0, -1), 0},
	[2]float64{0, Copysign(0, -1)},
	[2]float64{0, 0}, // +0, +0
	[2]float64{0, Inf(-1)},
	[2]float64{0, Inf(1)},
	[2]float64{0, NaN()},
	[2]float64{Inf(1), Inf(-1)},
	[2]float64{Inf(1), 0},
	[2]float64{Inf(1), Inf(1)},
	[2]float64{Inf(1), NaN()},
	[2]float64{NaN(), Inf(-1)},
	[2]float64{NaN(), 0},
	[2]float64{NaN(), Inf(1)},
	[2]float64{NaN(), NaN()},
}
var hypotSC = []float64{
	Inf(1),
	Inf(1),
	Inf(1),
	Inf(1),
	0,
	0,
	0,
	0,
	Inf(1),
	Inf(1),
	NaN(),
	Inf(1),
	Inf(1),
	Inf(1),
	Inf(1),
	Inf(1),
	NaN(),
	Inf(1),
	NaN(),
}

var vfilogbSC = []float64{
	Inf(-1),
	0,
	Inf(1),
	NaN(),
}
var ilogbSC = []int{
	MaxInt32,
	MinInt32,
	MaxInt32,
	MaxInt32,
}

var vfj0SC = []float64{
	Inf(-1),
	0,
	Inf(1),
	NaN(),
}
var j0SC = []float64{
	0,
	1,
	0,
	NaN(),
}
var j1SC = []float64{
	0,
	0,
	0,
	NaN(),
}
var j2SC = []float64{
	0,
	0,
	0,
	NaN(),
}
var jM3SC = []float64{
	0,
	0,
	0,
	NaN(),
}

var vflgammaSC = []float64{
	Inf(-1),
	-3,
	0,
	1,
	2,
	Inf(1),
	NaN(),
}
var lgammaSC = []fi{
	fi{Inf(-1), 1},
	fi{Inf(1), 1},
	fi{Inf(1), 1},
	fi{0, 1},
	fi{0, 1},
	fi{Inf(1), 1},
	fi{NaN(), 1},
}

var vflogSC = []float64{
	Inf(-1),
	-Pi,
	Copysign(0, -1),
	0,
	1,
	Inf(1),
	NaN(),
}
var logSC = []float64{
	NaN(),
	NaN(),
	Inf(-1),
	Inf(-1),
	0,
	Inf(1),
	NaN(),
}

var vflogbSC = []float64{
	Inf(-1),
	0,
	Inf(1),
	NaN(),
}
var logbSC = []float64{
	Inf(1),
	Inf(-1),
	Inf(1),
	NaN(),
}

var vflog1pSC = []float64{
	Inf(-1),
	-Pi,
	-1,
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var log1pSC = []float64{
	NaN(),
	NaN(),
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}

var vfmodfSC = []float64{
	Inf(-1),
	Inf(1),
	NaN(),
}
var modfSC = [][2]float64{
	[2]float64{Inf(-1), NaN()}, // [2]float64{Copysign(0, -1), Inf(-1)},
	[2]float64{Inf(1), NaN()},  // [2]float64{0, Inf(1)},
	[2]float64{NaN(), NaN()},
}

var vfnextafterSC = [][2]float64{
	[2]float64{0, NaN()},
	[2]float64{NaN(), 0},
	[2]float64{NaN(), NaN()},
}
var nextafterSC = []float64{
	NaN(),
	NaN(),
	NaN(),
}

var vfpowSC = [][2]float64{
	[2]float64{Inf(-1), -Pi},
	[2]float64{Inf(-1), -3},
	[2]float64{Inf(-1), Copysign(0, -1)},
	[2]float64{Inf(-1), 0},
	[2]float64{Inf(-1), 1},
	[2]float64{Inf(-1), 3},
	[2]float64{Inf(-1), Pi},
	[2]float64{Inf(-1), NaN()},

	[2]float64{-Pi, Inf(-1)},
	[2]float64{-Pi, -Pi},
	[2]float64{-Pi, Copysign(0, -1)},
	[2]float64{-Pi, 0},
	[2]float64{-Pi, 1},
	[2]float64{-Pi, Pi},
	[2]float64{-Pi, Inf(1)},
	[2]float64{-Pi, NaN()},

	[2]float64{-1, Inf(-1)},
	[2]float64{-1, Inf(1)},
	[2]float64{-1, NaN()},
	[2]float64{-1 / 2, Inf(-1)},
	[2]float64{-1 / 2, Inf(1)},
	[2]float64{Copysign(0, -1), Inf(-1)},
	[2]float64{Copysign(0, -1), -Pi},
	[2]float64{Copysign(0, -1), -3},
	[2]float64{Copysign(0, -1), 3},
	[2]float64{Copysign(0, -1), Pi},
	[2]float64{Copysign(0, -1), Inf(1)},

	[2]float64{0, Inf(-1)},
	[2]float64{0, -Pi},
	[2]float64{0, -3},
	[2]float64{0, Copysign(0, -1)},
	[2]float64{0, 0},
	[2]float64{0, 3},
	[2]float64{0, Pi},
	[2]float64{0, Inf(1)},
	[2]float64{0, NaN()},

	[2]float64{1 / 2, Inf(-1)},
	[2]float64{1 / 2, Inf(1)},
	[2]float64{1, Inf(-1)},
	[2]float64{1, Inf(1)},
	[2]float64{1, NaN()},

	[2]float64{Pi, Inf(-1)},
	[2]float64{Pi, Copysign(0, -1)},
	[2]float64{Pi, 0},
	[2]float64{Pi, 1},
	[2]float64{Pi, Inf(1)},
	[2]float64{Pi, NaN()},
	[2]float64{Inf(1), -Pi},
	[2]float64{Inf(1), Copysign(0, -1)},
	[2]float64{Inf(1), 0},
	[2]float64{Inf(1), 1},
	[2]float64{Inf(1), Pi},
	[2]float64{Inf(1), NaN()},
	[2]float64{NaN(), -Pi},
	[2]float64{NaN(), Copysign(0, -1)},
	[2]float64{NaN(), 0},
	[2]float64{NaN(), 1},
	[2]float64{NaN(), Pi},
	[2]float64{NaN(), NaN()},
}
var powSC = []float64{
	0,               // pow(-Inf, -Pi)
	Copysign(0, -1), // pow(-Inf, -3)
	1,               // pow(-Inf, -0)
	1,               // pow(-Inf, +0)
	Inf(-1),         // pow(-Inf, 1)
	Inf(-1),         // pow(-Inf, 3)
	Inf(1),          // pow(-Inf, Pi)
	NaN(),           // pow(-Inf, NaN)
	0,               // pow(-Pi, -Inf)
	NaN(),           // pow(-Pi, -Pi)
	1,               // pow(-Pi, -0)
	1,               // pow(-Pi, +0)
	-Pi,             // pow(-Pi, 1)
	NaN(),           // pow(-Pi, Pi)
	Inf(1),          // pow(-Pi, +Inf)
	NaN(),           // pow(-Pi, NaN)
	1,               // pow(-1, -Inf) IEEE 754-2008
	1,               // pow(-1, +Inf) IEEE 754-2008
	NaN(),           // pow(-1, NaN)
	Inf(1),          // pow(-1/2, -Inf)
	0,               // pow(-1/2, +Inf)
	Inf(1),          // pow(-0, -Inf)
	Inf(1),          // pow(-0, -Pi)
	Inf(-1),         // pow(-0, -3) IEEE 754-2008
	Copysign(0, -1), // pow(-0, 3) IEEE 754-2008
	0,               // pow(-0, +Pi)
	0,               // pow(-0, +Inf)
	Inf(1),          // pow(+0, -Inf)
	Inf(1),          // pow(+0, -Pi)
	Inf(1),          // pow(+0, -3)
	1,               // pow(+0, -0)
	1,               // pow(+0, +0)
	0,               // pow(+0, 3)
	0,               // pow(+0, +Pi)
	0,               // pow(+0, +Inf)
	NaN(),           // pow(+0, NaN)
	Inf(1),          // pow(1/2, -Inf)
	0,               // pow(1/2, +Inf)
	1,               // pow(1, -Inf) IEEE 754-2008
	1,               // pow(1, +Inf) IEEE 754-2008
	1,               // pow(1, NaN) IEEE 754-2008
	0,               // pow(+Pi, -Inf)
	1,               // pow(+Pi, -0)
	1,               // pow(+Pi, +0)
	Pi,              // pow(+Pi, 1)
	Inf(1),          // pow(+Pi, +Inf)
	NaN(),           // pow(+Pi, NaN)
	0,               // pow(+Inf, -Pi)
	1,               // pow(+Inf, -0)
	1,               // pow(+Inf, +0)
	Inf(1),          // pow(+Inf, 1)
	Inf(1),          // pow(+Inf, Pi)
	NaN(),           // pow(+Inf, NaN)
	NaN(),           // pow(NaN, -Pi)
	1,               // pow(NaN, -0)
	1,               // pow(NaN, +0)
	NaN(),           // pow(NaN, 1)
	NaN(),           // pow(NaN, +Pi)
	NaN(),           // pow(NaN, NaN)
}

var vfsignbitSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var signbitSC = []bool{
	true,
	true,
	false,
	false,
	false,
}

var vfsinSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var sinSC = []float64{
	NaN(),
	Copysign(0, -1),
	0,
	NaN(),
	NaN(),
}

var vfsinhSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var sinhSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}

var vfsqrtSC = []float64{
	Inf(-1),
	-Pi,
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var sqrtSC = []float64{
	NaN(),
	NaN(),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}

var vftanhSC = []float64{
	Inf(-1),
	Copysign(0, -1),
	0,
	Inf(1),
	NaN(),
}
var tanhSC = []float64{
	-1,
	Copysign(0, -1),
	0,
	1,
	NaN(),
}

var vfy0SC = []float64{
	Inf(-1),
	0,
	Inf(1),
	NaN(),
}
var y0SC = []float64{
	NaN(),
	Inf(-1),
	0,
	NaN(),
}
var y1SC = []float64{
	NaN(),
	Inf(-1),
	0,
	NaN(),
}
var y2SC = []float64{
	NaN(),
	Inf(-1),
	0,
	NaN(),
}
var yM3SC = []float64{
	NaN(),
	Inf(1),
	0,
	NaN(),
}

func tolerance(a, b, e float64) bool {
	d := a - b
	if d < 0 {
		d = -d
	}

	if a != 0 {
		e = e * a
		if e < 0 {
			e = -e
		}
	}
	return d < e
}
func kindaclose(a, b float64) bool { return tolerance(a, b, 1e-8) }
func close(a, b float64) bool      { return tolerance(a, b, 1e-14) }
func veryclose(a, b float64) bool  { return tolerance(a, b, 4e-16) }
func soclose(a, b, e float64) bool { return tolerance(a, b, e) }
func alike(a, b float64) bool {
	switch {
	case IsNaN(a) && IsNaN(b):
		return true
	case a == b:
		return Signbit(a) == Signbit(b)
	}
	return false
}

func TestAcos(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := vf[i] / 10
		if f := Acos(a); !close(acos[i], f) {
			t.Errorf("Acos(%g) = %g, want %g", a, f, acos[i])
		}
	}
	for i := 0; i < len(vfacosSC); i++ {
		if f := Acos(vfacosSC[i]); !alike(acosSC[i], f) {
			t.Errorf("Acos(%g) = %g, want %g", vfacosSC[i], f, acosSC[i])
		}
	}
}

func TestAcosh(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := 1 + Fabs(vf[i])
		if f := Acosh(a); !veryclose(acosh[i], f) {
			t.Errorf("Acosh(%g) = %g, want %g", a, f, acosh[i])
		}
	}
	for i := 0; i < len(vfacoshSC); i++ {
		if f := Acosh(vfacoshSC[i]); !alike(acoshSC[i], f) {
			t.Errorf("Acosh(%g) = %g, want %g", vfacoshSC[i], f, acoshSC[i])
		}
	}
}

func TestAsin(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := vf[i] / 10
		if f := Asin(a); !veryclose(asin[i], f) {
			t.Errorf("Asin(%g) = %g, want %g", a, f, asin[i])
		}
	}
	for i := 0; i < len(vfasinSC); i++ {
		if f := Asin(vfasinSC[i]); !alike(asinSC[i], f) {
			t.Errorf("Asin(%g) = %g, want %g", vfasinSC[i], f, asinSC[i])
		}
	}
}

func TestAsinh(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Asinh(vf[i]); !veryclose(asinh[i], f) {
			t.Errorf("Asinh(%g) = %g, want %g", vf[i], f, asinh[i])
		}
	}
	for i := 0; i < len(vfasinhSC); i++ {
		if f := Asinh(vfasinhSC[i]); !alike(asinhSC[i], f) {
			t.Errorf("Asinh(%g) = %g, want %g", vfasinhSC[i], f, asinhSC[i])
		}
	}
}

func TestAtan(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Atan(vf[i]); !veryclose(atan[i], f) {
			t.Errorf("Atan(%g) = %g, want %g", vf[i], f, atan[i])
		}
	}
	for i := 0; i < len(vfatanSC); i++ {
		if f := Atan(vfatanSC[i]); !alike(atanSC[i], f) {
			t.Errorf("Atan(%g) = %g, want %g", vfatanSC[i], f, atanSC[i])
		}
	}
}

func TestAtanh(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := vf[i] / 10
		if f := Atanh(a); !veryclose(atanh[i], f) {
			t.Errorf("Atanh(%g) = %g, want %g", a, f, atanh[i])
		}
	}
	for i := 0; i < len(vfatanhSC); i++ {
		if f := Atanh(vfatanhSC[i]); !alike(atanhSC[i], f) {
			t.Errorf("Atanh(%g) = %g, want %g", vfatanhSC[i], f, atanhSC[i])
		}
	}
}

func TestAtan2(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Atan2(10, vf[i]); !veryclose(atan2[i], f) {
			t.Errorf("Atan2(10, %g) = %g, want %g", vf[i], f, atan2[i])
		}
	}
	for i := 0; i < len(vfatan2SC); i++ {
		if f := Atan2(vfatan2SC[i][0], vfatan2SC[i][1]); !alike(atan2SC[i], f) {
			t.Errorf("Atan2(%g, %g) = %g, want %g", vfatan2SC[i][0], vfatan2SC[i][1], f, atan2SC[i])
		}
	}
}

func TestCbrt(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Cbrt(vf[i]); !veryclose(cbrt[i], f) {
			t.Errorf("Cbrt(%g) = %g, want %g", vf[i], f, cbrt[i])
		}
	}
	for i := 0; i < len(vfcbrtSC); i++ {
		if f := Cbrt(vfcbrtSC[i]); !alike(cbrtSC[i], f) {
			t.Errorf("Cbrt(%g) = %g, want %g", vfcbrtSC[i], f, cbrtSC[i])
		}
	}
}

func TestCeil(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Ceil(vf[i]); ceil[i] != f {
			t.Errorf("Ceil(%g) = %g, want %g", vf[i], f, ceil[i])
		}
	}
	for i := 0; i < len(vfceilSC); i++ {
		if f := Ceil(vfceilSC[i]); !alike(ceilSC[i], f) {
			t.Errorf("Ceil(%g) = %g, want %g", vfceilSC[i], f, ceilSC[i])
		}
	}
}

func TestCopysign(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Copysign(vf[i], -1); copysign[i] != f {
			t.Errorf("Copysign(%g, -1) = %g, want %g", vf[i], f, copysign[i])
		}
	}
	for i := 0; i < len(vf); i++ {
		if f := Copysign(vf[i], 1); -copysign[i] != f {
			t.Errorf("Copysign(%g, 1) = %g, want %g", vf[i], f, -copysign[i])
		}
	}
	for i := 0; i < len(vfcopysignSC); i++ {
		if f := Copysign(vfcopysignSC[i], -1); !alike(copysignSC[i], f) {
			t.Errorf("Copysign(%g, -1) = %g, want %g", vfcopysignSC[i], f, copysignSC[i])
		}
	}
}

func TestCos(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Cos(vf[i]); !close(cos[i], f) {
			t.Errorf("Cos(%g) = %g, want %g", vf[i], f, cos[i])
		}
	}
	for i := 0; i < len(vfcosSC); i++ {
		if f := Cos(vfcosSC[i]); !alike(cosSC[i], f) {
			t.Errorf("Cos(%g) = %g, want %g", vfcosSC[i], f, cosSC[i])
		}
	}
}

func TestCosh(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Cosh(vf[i]); !close(cosh[i], f) {
			t.Errorf("Cosh(%g) = %g, want %g", vf[i], f, cosh[i])
		}
	}
	for i := 0; i < len(vfcoshSC); i++ {
		if f := Cosh(vfcoshSC[i]); !alike(coshSC[i], f) {
			t.Errorf("Cosh(%g) = %g, want %g", vfcoshSC[i], f, coshSC[i])
		}
	}
}

func TestErf(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := vf[i] / 10
		if f := Erf(a); !veryclose(erf[i], f) {
			t.Errorf("Erf(%g) = %g, want %g", a, f, erf[i])
		}
	}
	for i := 0; i < len(vferfSC); i++ {
		if f := Erf(vferfSC[i]); !alike(erfSC[i], f) {
			t.Errorf("Erf(%g) = %g, want %g", vferfSC[i], f, erfSC[i])
		}
	}
}

func TestErfc(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := vf[i] / 10
		if f := Erfc(a); !veryclose(erfc[i], f) {
			t.Errorf("Erfc(%g) = %g, want %g", a, f, erfc[i])
		}
	}
	for i := 0; i < len(vferfcSC); i++ {
		if f := Erfc(vferfcSC[i]); !alike(erfcSC[i], f) {
			t.Errorf("Erfc(%g) = %g, want %g", vferfcSC[i], f, erfcSC[i])
		}
	}
}

func TestExp(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Exp(vf[i]); !close(exp[i], f) {
			t.Errorf("Exp(%g) = %g, want %g", vf[i], f, exp[i])
		}
	}
	for i := 0; i < len(vfexpSC); i++ {
		if f := Exp(vfexpSC[i]); !alike(expSC[i], f) {
			t.Errorf("Exp(%g) = %g, want %g", vfexpSC[i], f, expSC[i])
		}
	}
}

func TestExpm1(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := vf[i] / 100
		if f := Expm1(a); !veryclose(expm1[i], f) {
			t.Errorf("Expm1(%g) = %g, want %g", a, f, expm1[i])
		}
	}
	for i := 0; i < len(vfexpm1SC); i++ {
		if f := Expm1(vfexpm1SC[i]); !alike(expm1SC[i], f) {
			t.Errorf("Expm1(%g) = %g, want %g", vfexpm1SC[i], f, expm1SC[i])
		}
	}
}

func TestExp2(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Exp2(vf[i]); !close(exp2[i], f) {
			t.Errorf("Exp2(%g) = %g, want %g", vf[i], f, exp2[i])
		}
	}
	for i := 0; i < len(vfexpSC); i++ {
		if f := Exp2(vfexpSC[i]); !alike(expSC[i], f) {
			t.Errorf("Exp2(%g) = %g, want %g", vfexpSC[i], f, expSC[i])
		}
	}
}

func TestFabs(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Fabs(vf[i]); fabs[i] != f {
			t.Errorf("Fabs(%g) = %g, want %g", vf[i], f, fabs[i])
		}
	}
	for i := 0; i < len(vffabsSC); i++ {
		if f := Fabs(vffabsSC[i]); !alike(fabsSC[i], f) {
			t.Errorf("Fabs(%g) = %g, want %g", vffabsSC[i], f, fabsSC[i])
		}
	}
}

func TestFdim(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Fdim(vf[i], 0); fdim[i] != f {
			t.Errorf("Fdim(%g, %g) = %g, want %g", vf[i], 0.0, f, fdim[i])
		}
	}
}

func TestFloor(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Floor(vf[i]); floor[i] != f {
			t.Errorf("Floor(%g) = %g, want %g", vf[i], f, floor[i])
		}
	}
	for i := 0; i < len(vfceilSC); i++ {
		if f := Floor(vfceilSC[i]); !alike(ceilSC[i], f) {
			t.Errorf("Floor(%g) = %g, want %g", vfceilSC[i], f, ceilSC[i])
		}
	}
}

func TestFmax(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Fmax(vf[i], ceil[i]); ceil[i] != f {
			t.Errorf("Fmax(%g, %g) = %g, want %g", vf[i], ceil[i], f, ceil[i])
		}
	}
}

func TestFmin(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Fmin(vf[i], floor[i]); floor[i] != f {
			t.Errorf("Fmin(%g, %g) = %g, want %g", vf[i], floor[i], f, floor[i])
		}
	}
}

func TestFmod(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Fmod(10, vf[i]); fmod[i] != f {
			t.Errorf("Fmod(10, %g) = %g, want %g", vf[i], f, fmod[i])
		}
	}
	for i := 0; i < len(vffmodSC); i++ {
		if f := Fmod(vffmodSC[i][0], vffmodSC[i][1]); !alike(fmodSC[i], f) {
			t.Errorf("Fmod(%g, %g) = %g, want %g", vffmodSC[i][0], vffmodSC[i][1], f, fmodSC[i])
		}
	}
}

func TestFrexp(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f, j := Frexp(vf[i]); !veryclose(frexp[i].f, f) || frexp[i].i != j {
			t.Errorf("Frexp(%g) = %g, %d, want %g, %d", vf[i], f, j, frexp[i].f, frexp[i].i)
		}
	}
	for i := 0; i < len(vffrexpSC); i++ {
		if f, j := Frexp(vffrexpSC[i]); !alike(frexpSC[i].f, f) || frexpSC[i].i != j {
			t.Errorf("Frexp(%g) = %g, %d, want %g, %d", vffrexpSC[i], f, j, frexpSC[i].f, frexpSC[i].i)
		}
	}
}

func TestGamma(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Gamma(vf[i]); !close(gamma[i], f) {
			t.Errorf("Gamma(%g) = %g, want %g", vf[i], f, gamma[i])
		}
	}
	for i := 0; i < len(vfgammaSC); i++ {
		if f := Gamma(vfgammaSC[i]); !alike(gammaSC[i], f) {
			t.Errorf("Gamma(%g) = %g, want %g", vfgammaSC[i], f, gammaSC[i])
		}
	}
}

func TestHypot(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(1e200 * tanh[i] * Sqrt(2))
		if f := Hypot(1e200*tanh[i], 1e200*tanh[i]); !veryclose(a, f) {
			t.Errorf("Hypot(%g, %g) = %g, want %g", 1e200*tanh[i], 1e200*tanh[i], f, a)
		}
	}
	for i := 0; i < len(vfhypotSC); i++ {
		if f := Hypot(vfhypotSC[i][0], vfhypotSC[i][1]); !alike(hypotSC[i], f) {
			t.Errorf("Hypot(%g, %g) = %g, want %g", vfhypotSC[i][0], vfhypotSC[i][1], f, hypotSC[i])
		}
	}
}

func TestIlogb(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := frexp[i].i - 1 // adjust because fr in the interval [½, 1)
		if e := Ilogb(vf[i]); a != e {
			t.Errorf("Ilogb(%g) = %d, want %d", vf[i], e, a)
		}
	}
	for i := 0; i < len(vflogbSC); i++ {
		if e := Ilogb(vflogbSC[i]); ilogbSC[i] != e {
			t.Errorf("Ilogb(%g) = %d, want %d", vflogbSC[i], e, ilogbSC[i])
		}
	}
}

func TestJ0(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := J0(vf[i]); !soclose(j0[i], f, 4e-14) {
			t.Errorf("J0(%g) = %g, want %g", vf[i], f, j0[i])
		}
	}
	for i := 0; i < len(vfj0SC); i++ {
		if f := J0(vfj0SC[i]); !alike(j0SC[i], f) {
			t.Errorf("J0(%g) = %g, want %g", vfj0SC[i], f, j0SC[i])
		}
	}
}

func TestJ1(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := J1(vf[i]); !close(j1[i], f) {
			t.Errorf("J1(%g) = %g, want %g", vf[i], f, j1[i])
		}
	}
	for i := 0; i < len(vfj0SC); i++ {
		if f := J1(vfj0SC[i]); !alike(j1SC[i], f) {
			t.Errorf("J1(%g) = %g, want %g", vfj0SC[i], f, j1SC[i])
		}
	}
}

func TestJn(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Jn(2, vf[i]); !close(j2[i], f) {
			t.Errorf("Jn(2, %g) = %g, want %g", vf[i], f, j2[i])
		}
		if f := Jn(-3, vf[i]); !close(jM3[i], f) {
			t.Errorf("Jn(-3, %g) = %g, want %g", vf[i], f, jM3[i])
		}
	}
	for i := 0; i < len(vfj0SC); i++ {
		if f := Jn(2, vfj0SC[i]); !alike(j2SC[i], f) {
			t.Errorf("Jn(2, %g) = %g, want %g", vfj0SC[i], f, j2SC[i])
		}
		if f := Jn(-3, vfj0SC[i]); !alike(jM3SC[i], f) {
			t.Errorf("Jn(-3, %g) = %g, want %g", vfj0SC[i], f, jM3SC[i])
		}
	}
}

func TestLdexp(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Ldexp(frexp[i].f, frexp[i].i); !veryclose(vf[i], f) {
			t.Errorf("Ldexp(%g, %d) = %g, want %g", frexp[i].f, frexp[i].i, f, vf[i])
		}
	}
	for i := 0; i < len(vffrexpSC); i++ {
		if f := Ldexp(frexpSC[i].f, frexpSC[i].i); !alike(vffrexpSC[i], f) {
			t.Errorf("Ldexp(%g, %d) = %g, want %g", frexpSC[i].f, frexpSC[i].i, f, vffrexpSC[i])
		}
	}
}

func TestLgamma(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f, s := Lgamma(vf[i]); !close(lgamma[i].f, f) || lgamma[i].i != s {
			t.Errorf("Lgamma(%g) = %g, %d, want %g, %d", vf[i], f, s, lgamma[i].f, lgamma[i].i)
		}
	}
	for i := 0; i < len(vflgammaSC); i++ {
		if f, s := Lgamma(vflgammaSC[i]); !alike(lgammaSC[i].f, f) || lgammaSC[i].i != s {
			t.Errorf("Lgamma(%g) = %g, %d, want %g, %d", vflgammaSC[i], f, s, lgammaSC[i].f, lgammaSC[i].i)
		}
	}
}

func TestLog(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(vf[i])
		if f := Log(a); log[i] != f {
			t.Errorf("Log(%g) = %g, want %g", a, f, log[i])
		}
	}
	if f := Log(10); f != Ln10 {
		t.Errorf("Log(%g) = %g, want %g", 10.0, f, Ln10)
	}
	for i := 0; i < len(vflogSC); i++ {
		if f := Log(vflogSC[i]); !alike(logSC[i], f) {
			t.Errorf("Log(%g) = %g, want %g", vflogSC[i], f, logSC[i])
		}
	}
}

func TestLogb(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Logb(vf[i]); logb[i] != f {
			t.Errorf("Logb(%g) = %g, want %g", vf[i], f, logb[i])
		}
	}
	for i := 0; i < len(vflogbSC); i++ {
		if f := Logb(vflogbSC[i]); !alike(logbSC[i], f) {
			t.Errorf("Logb(%g) = %g, want %g", vflogbSC[i], f, logbSC[i])
		}
	}
}

func TestLog10(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(vf[i])
		if f := Log10(a); !veryclose(log10[i], f) {
			t.Errorf("Log10(%g) = %g, want %g", a, f, log10[i])
		}
	}
	if f := Log10(E); f != Log10E {
		t.Errorf("Log10(%g) = %g, want %g", E, f, Log10E)
	}
	for i := 0; i < len(vflogSC); i++ {
		if f := Log10(vflogSC[i]); !alike(logSC[i], f) {
			t.Errorf("Log10(%g) = %g, want %g", vflogSC[i], f, logSC[i])
		}
	}
}

func TestLog1p(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := vf[i] / 100
		if f := Log1p(a); !veryclose(log1p[i], f) {
			t.Errorf("Log1p(%g) = %g, want %g", a, f, log1p[i])
		}
	}
	a := float64(9)
	if f := Log1p(a); f != Ln10 {
		t.Errorf("Log1p(%g) = %g, want %g", a, f, Ln10)
	}
	for i := 0; i < len(vflogSC); i++ {
		if f := Log1p(vflog1pSC[i]); !alike(log1pSC[i], f) {
			t.Errorf("Log1p(%g) = %g, want %g", vflog1pSC[i], f, log1pSC[i])
		}
	}
}

func TestLog2(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(vf[i])
		if f := Log2(a); !veryclose(log2[i], f) {
			t.Errorf("Log2(%g) = %g, want %g", a, f, log2[i])
		}
	}
	if f := Log2(E); f != Log2E {
		t.Errorf("Log2(%g) = %g, want %g", E, f, Log2E)
	}
	for i := 0; i < len(vflogSC); i++ {
		if f := Log2(vflogSC[i]); !alike(logSC[i], f) {
			t.Errorf("Log2(%g) = %g, want %g", vflogSC[i], f, logSC[i])
		}
	}
}

func TestModf(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f, g := Modf(vf[i]); !veryclose(modf[i][0], f) || !veryclose(modf[i][1], g) {
			t.Errorf("Modf(%g) = %g, %g, want %g, %g", vf[i], f, g, modf[i][0], modf[i][1])
		}
	}
	for i := 0; i < len(vfmodfSC); i++ {
		if f, g := Modf(vfmodfSC[i]); !alike(modfSC[i][0], f) || !alike(modfSC[i][1], g) {
			t.Errorf("Modf(%g) = %g, %g, want %g, %g", vfmodfSC[i], f, g, modfSC[i][0], modfSC[i][1])
		}
	}
}

func TestNextafter(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Nextafter(vf[i], 10); nextafter[i] != f {
			t.Errorf("Nextafter(%g, %g) = %g want %g", vf[i], 10.0, f, nextafter[i])
		}
	}
	for i := 0; i < len(vfmodfSC); i++ {
		if f := Nextafter(vfnextafterSC[i][0], vfnextafterSC[i][1]); !alike(nextafterSC[i], f) {
			t.Errorf("Nextafter(%g, %g) = %g want %g", vfnextafterSC[i][0], vfnextafterSC[i][1], f, nextafterSC[i])
		}
	}
}

func TestPow(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Pow(10, vf[i]); !close(pow[i], f) {
			t.Errorf("Pow(10, %g) = %g, want %g", vf[i], f, pow[i])
		}
	}
	for i := 0; i < len(vfpowSC); i++ {
		if f := Pow(vfpowSC[i][0], vfpowSC[i][1]); !alike(powSC[i], f) {
			t.Errorf("Pow(%g, %g) = %g, want %g", vfpowSC[i][0], vfpowSC[i][1], f, powSC[i])
		}
	}
}

func TestRemainder(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Remainder(10, vf[i]); remainder[i] != f {
			t.Errorf("Remainder(10, %g) = %g, want %g", vf[i], f, remainder[i])
		}
	}
	for i := 0; i < len(vffmodSC); i++ {
		if f := Remainder(vffmodSC[i][0], vffmodSC[i][1]); !alike(fmodSC[i], f) {
			t.Errorf("Remainder(%g, %g) = %g, want %g", vffmodSC[i][0], vffmodSC[i][1], f, fmodSC[i])
		}
	}
}

func TestSignbit(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Signbit(vf[i]); signbit[i] != f {
			t.Errorf("Signbit(%g) = %t, want %t", vf[i], f, signbit[i])
		}
	}
	for i := 0; i < len(vfsignbitSC); i++ {
		if f := Signbit(vfsignbitSC[i]); signbitSC[i] != f {
			t.Errorf("Signbit(%g) = %t, want %t", vfsignbitSC[i], f, signbitSC[i])
		}
	}
}
func TestSin(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Sin(vf[i]); !close(sin[i], f) {
			t.Errorf("Sin(%g) = %g, want %g", vf[i], f, sin[i])
		}
	}
	for i := 0; i < len(vfsinSC); i++ {
		if f := Sin(vfsinSC[i]); !alike(sinSC[i], f) {
			t.Errorf("Sin(%g) = %g, want %g", vfsinSC[i], f, sinSC[i])
		}
	}
}

func TestSincos(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if s, c := Sincos(vf[i]); !close(sin[i], s) || !close(cos[i], c) {
			t.Errorf("Sincos(%g) = %g, %g want %g, %g", vf[i], s, c, sin[i], cos[i])
		}
	}
}

func TestSinh(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Sinh(vf[i]); !close(sinh[i], f) {
			t.Errorf("Sinh(%g) = %g, want %g", vf[i], f, sinh[i])
		}
	}
	for i := 0; i < len(vfsinhSC); i++ {
		if f := Sinh(vfsinhSC[i]); !alike(sinhSC[i], f) {
			t.Errorf("Sinh(%g) = %g, want %g", vfsinhSC[i], f, sinhSC[i])
		}
	}
}

func TestSqrt(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(vf[i])
		if f := SqrtGo(a); sqrt[i] != f {
			t.Errorf("SqrtGo(%g) = %g, want %g", a, f, sqrt[i])
		}
		a = Fabs(vf[i])
		if f := Sqrt(a); sqrt[i] != f {
			t.Errorf("Sqrt(%g) = %g, want %g", a, f, sqrt[i])
		}
	}
	for i := 0; i < len(vfsqrtSC); i++ {
		if f := SqrtGo(vfsqrtSC[i]); !alike(sqrtSC[i], f) {
			t.Errorf("SqrtGo(%g) = %g, want %g", vfsqrtSC[i], f, sqrtSC[i])
		}
		if f := Sqrt(vfsqrtSC[i]); !alike(sqrtSC[i], f) {
			t.Errorf("Sqrt(%g) = %g, want %g", vfsqrtSC[i], f, sqrtSC[i])
		}
	}
}

func TestTan(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Tan(vf[i]); !close(tan[i], f) {
			t.Errorf("Tan(%g) = %g, want %g", vf[i], f, tan[i])
		}
	}
	// same special cases as Sin
	for i := 0; i < len(vfsinSC); i++ {
		if f := Tan(vfsinSC[i]); !alike(sinSC[i], f) {
			t.Errorf("Tan(%g) = %g, want %g", vfsinSC[i], f, sinSC[i])
		}
	}

	// Make sure portable Tan(Pi/2) doesn't panic (it used to).
	// The portable implementation returns NaN.
	// Assembly implementations might not,
	// because Pi/2 is not exactly representable.
	if runtime.GOARCH != "386" {
		if f := Tan(Pi / 2); !alike(f, NaN()) {
			t.Errorf("Tan(%g) = %g, want %g", Pi/2, f, NaN())
		}
	}
}

func TestTanh(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Tanh(vf[i]); !veryclose(tanh[i], f) {
			t.Errorf("Tanh(%g) = %g, want %g", vf[i], f, tanh[i])
		}
	}
	for i := 0; i < len(vftanhSC); i++ {
		if f := Tanh(vftanhSC[i]); !alike(tanhSC[i], f) {
			t.Errorf("Tanh(%g) = %g, want %g", vftanhSC[i], f, tanhSC[i])
		}
	}
}

func TestTrunc(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		if f := Trunc(vf[i]); trunc[i] != f {
			t.Errorf("Trunc(%g) = %g, want %g", vf[i], f, trunc[i])
		}
	}
	for i := 0; i < len(vfceilSC); i++ {
		if f := Trunc(vfceilSC[i]); !alike(ceilSC[i], f) {
			t.Errorf("Trunc(%g) = %g, want %g", vfceilSC[i], f, ceilSC[i])
		}
	}
}

func TestY0(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(vf[i])
		if f := Y0(a); !close(y0[i], f) {
			t.Errorf("Y0(%g) = %g, want %g", a, f, y0[i])
		}
	}
	for i := 0; i < len(vfy0SC); i++ {
		if f := Y0(vfy0SC[i]); !alike(y0SC[i], f) {
			t.Errorf("Y0(%g) = %g, want %g", vfy0SC[i], f, y0SC[i])
		}
	}
}

func TestY1(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(vf[i])
		if f := Y1(a); !soclose(y1[i], f, 2e-14) {
			t.Errorf("Y1(%g) = %g, want %g", a, f, y1[i])
		}
	}
	for i := 0; i < len(vfy0SC); i++ {
		if f := Y1(vfy0SC[i]); !alike(y1SC[i], f) {
			t.Errorf("Y1(%g) = %g, want %g", vfy0SC[i], f, y1SC[i])
		}
	}
}

func TestYn(t *testing.T) {
	for i := 0; i < len(vf); i++ {
		a := Fabs(vf[i])
		if f := Yn(2, a); !close(y2[i], f) {
			t.Errorf("Yn(2, %g) = %g, want %g", a, f, y2[i])
		}
		if f := Yn(-3, a); !close(yM3[i], f) {
			t.Errorf("Yn(-3, %g) = %g, want %g", a, f, yM3[i])
		}
	}
	for i := 0; i < len(vfy0SC); i++ {
		if f := Yn(2, vfy0SC[i]); !alike(y2SC[i], f) {
			t.Errorf("Yn(2, %g) = %g, want %g", vfy0SC[i], f, y2SC[i])
		}
		if f := Yn(-3, vfy0SC[i]); !alike(yM3SC[i], f) {
			t.Errorf("Yn(-3, %g) = %g, want %g", vfy0SC[i], f, yM3SC[i])
		}
	}
}

// Check that math functions of high angle values
// return similar results to low angle values
func TestLargeCos(t *testing.T) {
	large := float64(100000 * Pi)
	for i := 0; i < len(vf); i++ {
		f1 := Cos(vf[i])
		f2 := Cos(vf[i] + large)
		if !kindaclose(f1, f2) {
			t.Errorf("Cos(%g) = %g, want %g", vf[i]+large, f2, f1)
		}
	}
}

func TestLargeSin(t *testing.T) {
	large := float64(100000 * Pi)
	for i := 0; i < len(vf); i++ {
		f1 := Sin(vf[i])
		f2 := Sin(vf[i] + large)
		if !kindaclose(f1, f2) {
			t.Errorf("Sin(%g) = %g, want %g", vf[i]+large, f2, f1)
		}
	}
}

func TestLargeSincos(t *testing.T) {
	large := float64(100000 * Pi)
	for i := 0; i < len(vf); i++ {
		f1, g1 := Sincos(vf[i])
		f2, g2 := Sincos(vf[i] + large)
		if !kindaclose(f1, f2) || !kindaclose(g1, g2) {
			t.Errorf("Sincos(%g) = %g, %g, want %g, %g", vf[i]+large, f2, g2, f1, g1)
		}
	}
}

func TestLargeTan(t *testing.T) {
	large := float64(100000 * Pi)
	for i := 0; i < len(vf); i++ {
		f1 := Tan(vf[i])
		f2 := Tan(vf[i] + large)
		if !kindaclose(f1, f2) {
			t.Errorf("Tan(%g) = %g, want %g", vf[i]+large, f2, f1)
		}
	}
}

// Check that math constants are accepted by compiler
// and have right value (assumes strconv.Atof works).
// http://code.google.com/p/go/issues/detail?id=201

type floatTest struct {
	val  interface{}
	name string
	str  string
}

var floatTests = []floatTest{
	floatTest{float64(MaxFloat64), "MaxFloat64", "1.7976931348623157e+308"},
	floatTest{float64(MinFloat64), "MinFloat64", "5e-324"},
	floatTest{float32(MaxFloat32), "MaxFloat32", "3.4028235e+38"},
	floatTest{float32(MinFloat32), "MinFloat32", "1e-45"},
}

func TestFloatMinMax(t *testing.T) {
	for _, tt := range floatTests {
		s := fmt.Sprint(tt.val)
		if s != tt.str {
			t.Errorf("Sprint(%v) = %s, want %s", tt.name, s, tt.str)
		}
	}
}

// Benchmarks

func BenchmarkAcos(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Acos(.5)
	}
}

func BenchmarkAcosh(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Acosh(1.5)
	}
}

func BenchmarkAsin(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Asin(.5)
	}
}

func BenchmarkAsinh(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Asinh(.5)
	}
}

func BenchmarkAtan(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Atan(.5)
	}
}

func BenchmarkAtanh(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Atanh(.5)
	}
}

func BenchmarkAtan2(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Atan2(.5, 1)
	}
}

func BenchmarkCbrt(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Cbrt(10)
	}
}

func BenchmarkCeil(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Ceil(.5)
	}
}

func BenchmarkCopysign(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Copysign(.5, -1)
	}
}

func BenchmarkCos(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Cos(.5)
	}
}

func BenchmarkCosh(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Cosh(2.5)
	}
}

func BenchmarkErf(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Erf(.5)
	}
}

func BenchmarkErfc(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Erfc(.5)
	}
}

func BenchmarkExp(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Exp(.5)
	}
}

func BenchmarkExpm1(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Expm1(.5)
	}
}

func BenchmarkExp2(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Exp2(.5)
	}
}

func BenchmarkFabs(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Fabs(.5)
	}
}

func BenchmarkFdim(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Fdim(10, 3)
	}
}

func BenchmarkFloor(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Floor(.5)
	}
}

func BenchmarkFmax(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Fmax(10, 3)
	}
}

func BenchmarkFmin(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Fmin(10, 3)
	}
}

func BenchmarkFmod(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Fmod(10, 3)
	}
}

func BenchmarkFrexp(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Frexp(8)
	}
}

func BenchmarkGamma(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Gamma(2.5)
	}
}

func BenchmarkHypot(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Hypot(3, 4)
	}
}

func BenchmarkHypotGo(b *testing.B) {
	for i := 0; i < b.N; i++ {
		HypotGo(3, 4)
	}
}

func BenchmarkIlogb(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Ilogb(.5)
	}
}

func BenchmarkJ0(b *testing.B) {
	for i := 0; i < b.N; i++ {
		J0(2.5)
	}
}

func BenchmarkJ1(b *testing.B) {
	for i := 0; i < b.N; i++ {
		J1(2.5)
	}
}

func BenchmarkJn(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Jn(2, 2.5)
	}
}

func BenchmarkLdexp(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Ldexp(.5, 2)
	}
}

func BenchmarkLgamma(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Lgamma(2.5)
	}
}

func BenchmarkLog(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Log(.5)
	}
}

func BenchmarkLogb(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Logb(.5)
	}
}

func BenchmarkLog1p(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Log1p(.5)
	}
}

func BenchmarkLog10(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Log10(.5)
	}
}

func BenchmarkLog2(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Log2(.5)
	}
}

func BenchmarkModf(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Modf(1.5)
	}
}

func BenchmarkNextafter(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Nextafter(.5, 1)
	}
}

func BenchmarkPowInt(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Pow(2, 2)
	}
}

func BenchmarkPowFrac(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Pow(2.5, 1.5)
	}
}

func BenchmarkRemainder(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Remainder(10, 3)
	}
}

func BenchmarkSignbit(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Signbit(2.5)
	}
}

func BenchmarkSin(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Sin(.5)
	}
}

func BenchmarkSincos(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Sincos(.5)
	}
}

func BenchmarkSinh(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Sinh(2.5)
	}
}

func BenchmarkSqrt(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Sqrt(10)
	}
}

func BenchmarkSqrtGo(b *testing.B) {
	for i := 0; i < b.N; i++ {
		SqrtGo(10)
	}
}

func BenchmarkTan(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Tan(.5)
	}
}

func BenchmarkTanh(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Tanh(2.5)
	}
}
func BenchmarkTrunc(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Trunc(.5)
	}
}

func BenchmarkY0(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Y0(2.5)
	}
}

func BenchmarkY1(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Y1(2.5)
	}
}

func BenchmarkYn(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Yn(2, 2.5)
	}
}
