angular.module('sqrt', ["kendo.directives"])
    .controller('MainController', function( $scope){
        $scope.model = window.model;
        setInterval(function(){
            $scope.model.heartRate = window.model.heartRate;
            $scope.model.bac = window.model.bac;
            $scope.$apply()
        }, 100);
    })
    .controller('HeartController', function($scope){
        $scope.heartScale ={
            minorUnit: 5,
            startAngle: -30,
            endAngle: 210,
            min: 50,
            max: 180,
            labels: {
                position: "inside"
            },
            ranges: [
                {
                    from: 50,
                    to: 100,
                    color: "#00ff00"
                }
                , {
                    from: 100,
                    to: 135,
                    color: "#ff7a00"
                }, {
                    from: 135,
                    to: 180,
                    color: "#ff0000"
                }
            ]
        };

    })
    .controller('BACController', function($scope){
    })
    .controller('FaceController', function($scope){
    });