from django.core.mail import send_mail
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
import json
import logging

# Configure logging
logger = logging.getLogger(__name__)

@csrf_exempt
def sensor_data(request):
    if request.method == 'POST':
        try:
            data = json.loads(request.body)
            distance1 = data.get('distance1', None)
            distance2 = data.get('distance2', None)
            
            if distance1 is not None and distance2 is not None:
                # Print the distances to the console
                print(f"Received distance1: {distance1} cm, distance2: {distance2} cm")

                # Send the email
                send_mail(
                    'Distance Sensor Data',
                    f'The distances measured by the sensors are:\nSensor 1: {distance1} cm\nSensor 2: {distance2} cm',
                    'lusuku2002@gmail.com',  # From email
                    ['isaac256kiwa@gmail.com'],  # To email
                    fail_silently=False,
                )

                return JsonResponse({"status": "success", "distance1": distance1, "distance2": distance2})
            else:
                return JsonResponse({"status": "error", "message": "Distances not provided"}, status=400)
        except json.JSONDecodeError:
            logger.error("Invalid JSON format.")
            return JsonResponse({"status": "error", "message": "Invalid JSON"}, status=400)
        except Exception as e:
            logger.error(f"An unexpected error occurred: {e}")
            return JsonResponse({"status": "error", "message": "Internal server error"}, status=500)
    else:
        return JsonResponse({"status": "error", "message": "Invalid request method"}, status=405)
